#include "WorldState.h"
#include "EvaluateTemplate.h"
#include "FormCallbacks.h"
#include "LeveledListUtils.h"
#include "LocationalDataUtils.h"
#include "MpActor.h"
#include "MpChangeForms.h"
#include "MpObjectReference.h"
#include "ScopedTask.h"
#include "Timer.h"
#include "libespm/GroupUtils.h"
#include "papyrus-vm/Reader.h"
#include "papyrus-vm/Utils.h"
#include "save_storages/ISaveStorage.h"
#include "script_classes/PapyrusClassesFactory.h"
#include "script_compatibility_policies/PapyrusCompatibilityPolicyFactory.h"
#include "script_storages/IScriptStorage.h"
#include <algorithm>
#include <deque>
#include <iterator>
#include <unordered_map>

struct WorldState::Impl
{
  std::unordered_map<uint32_t, MpChangeForm> changes;
  std::shared_ptr<ISaveStorage> saveStorage;
  std::shared_ptr<IScriptStorage> scriptStorage;
  bool saveStorageBusy = false;
  std::shared_ptr<VirtualMachine> vm;
  uint32_t nextId = 0xff000000;
  std::shared_ptr<IPapyrusCompatibilityPolicy> policy;
  std::unordered_map<uint32_t, MpChangeForm> changeFormsForDeferredLoad;
  bool chunkLoadingInProgress = false;
  bool formLoadingInProgress = false;
  std::map<std::string, std::chrono::system_clock::duration>
    relootTimeForTypes;
  std::set<std::string> forbiddenRelootTypes;
  std::vector<std::unique_ptr<IPapyrusClassBase>> classes;
};

WorldState::WorldState()
{
  logger.reset(new spdlog::logger("empty logger"));

  pImpl.reset(new Impl);
  pImpl->policy = PapyrusCompatibilityPolicyFactory::Create(this);
}

void WorldState::Clear()
{
  forms.clear();
  grids.clear();
  formIdxManager.reset();
}

void WorldState::AttachEspm(espm::Loader* espm_,
                            const FormCallbacksFactory& formCallbacksFactory_)
{
  espm = espm_;
  formCallbacksFactory = formCallbacksFactory_;
  espmCache.reset(new espm::CompressedFieldsCache);
  espmFiles = espm->GetFileNames();
}

void WorldState::AttachSaveStorage(std::shared_ptr<ISaveStorage> saveStorage)
{
  pImpl->saveStorage = saveStorage;
}

void WorldState::AttachScriptStorage(
  std::shared_ptr<IScriptStorage> scriptStorage)
{
  pImpl->scriptStorage = scriptStorage;
}

void WorldState::AddForm(std::unique_ptr<MpForm> form, uint32_t formId,
                         bool skipChecks,
                         const MpChangeForm* optionalChangeFormToApply)
{
  if (!skipChecks && forms.find(formId) != forms.end()) {

    throw std::runtime_error(
      static_cast<const std::stringstream&>(std::stringstream()
                                            << "Form with id " << std::hex
                                            << formId << " already exists")
        .str());
  }
  form->Init(this, formId, optionalChangeFormToApply != nullptr);

  if (auto formIndex = dynamic_cast<FormIndex*>(form.get())) {
    if (!formIdxManager)
      formIdxManager.reset(new MakeID(FormIndex::g_invalidIdx - 1));
    if (!formIdxManager->CreateID(formIndex->idx))
      throw std::runtime_error("CreateID failed");

    if (formByIdxUnreliable.size() <= formIndex->idx)
      formByIdxUnreliable.resize(formIndex->idx + 1, nullptr);
    formByIdxUnreliable[formIndex->idx] = form.get();
  }

  auto it = forms.insert({ formId, std::move(form) }).first;

  if (optionalChangeFormToApply) {
    auto refr = dynamic_cast<MpObjectReference*>(it->second.get());
    if (!refr) {
      forms.erase(it); // Rollback changes due to exception
      throw std::runtime_error(
        "Unable to apply ChangeForm, cast to ObjectReference failed");
    }
    refr->ApplyChangeForm(*optionalChangeFormToApply);
  }
}

void WorldState::Tick()
{
  const auto now = std::chrono::system_clock::now();
  TickReloot(now);
  TickSaveStorage(now);
  TickTimers(now);
}

void WorldState::LoadChangeForm(const MpChangeForm& changeForm,
                                const FormCallbacks& callbacks)
{
  Viet::ScopedTask<bool> task([](bool& st) { st = false; },
                              pImpl->formLoadingInProgress);
  pImpl->formLoadingInProgress = true;

  std::unique_ptr<MpObjectReference> form;

  const auto baseId = changeForm.baseDesc.ToFormId(espmFiles);
  const auto formId = changeForm.formDesc.ToFormId(espmFiles);

  std::string baseType = "STAT";
  if (espm) {
    const auto rec = espm->GetBrowser().LookupById(baseId).rec;

    if (!rec) {
      spdlog::warn("Skipping ChangeForm {:x}: unable to find base record {:x}",
                   formId, baseId);
      return;
    }
    baseType = rec->GetType().ToString();
  }

  if (espm) {
    if (!changeForm.appearanceDump.empty()) {
      simdjson::dom::parser p;
      auto res = p.parse(changeForm.appearanceDump);
      if (res.error()) {
        spdlog::warn(
          "Skipping ChangeForm {:x}: unable to parse appearanceDump", formId);
        return;
      }

      Appearance appearance;
      try {
        appearance = Appearance::FromJson(res.value());
      } catch (std::exception& e) {
        spdlog::warn("Skipping ChangeForm {:x}: Appearance::FromJson failed "
                     "on appearanceDump: {}",
                     formId, e.what());
        return;
      }

      const auto rec = espm->GetBrowser().LookupById(appearance.raceId).rec;

      if (!rec || rec->GetType().ToString() != "RACE") {
        spdlog::warn("Skipping ChangeForm {:x}: bad raceId in appearanceDump",
                     formId);
        return;
      }
    }
  }

  if (formId < 0xff000000) {
    auto it = forms.find(formId);
    if (it != forms.end()) {
      auto refr = std::dynamic_pointer_cast<MpObjectReference>(it->second);
      if (refr) {
        refr->ApplyChangeForm(changeForm);
      }
    } else {
      pImpl->changeFormsForDeferredLoad[formId] = changeForm;
    }
    return;
  }

  switch (changeForm.recType) {
    case MpChangeForm::ACHR:
      form.reset(new MpActor(LocationalData(), callbacks, baseId));
      break;
    case MpChangeForm::REFR:
      form.reset(new MpObjectReference(LocationalData(), callbacks, baseId,
                                       baseType.data()));
      break;
    default:
      throw std::runtime_error("Unknown ChangeForm type: " +
                               std::to_string(changeForm.recType));
  }

  AddForm(std::move(form), formId, false, &changeForm);

  // EnsureBaseContainerAdded forces saving here.
  // We do not want characters to save when they are load partially
  // This behaviour results in
  // https://github.com/skyrim-multiplayer/issue-tracker/issues/64

  // So we expect that RequestSave does nothing in this case:
  assert(pImpl->changes.count(formId) == 0);

  // For Release configuration we just manually remove formId from changes
  pImpl->changes.erase(formId);
}

void WorldState::RequestReloot(MpObjectReference& ref,
                               std::chrono::system_clock::duration time)
{
  auto& list = relootTimers[time];
  list.push_back({ ref.GetFormId(), std::chrono::system_clock::now() + time });
}

void WorldState::RequestSave(MpObjectReference& ref)
{
  if (!pImpl->formLoadingInProgress) {
    pImpl->changes[ref.GetFormId()] = ref.GetChangeForm();
  }
}

const std::shared_ptr<MpForm>& WorldState::LookupFormById(
  uint32_t formId, std::stringstream* optionalOutTrace)
{
  static const std::shared_ptr<MpForm> kNullForm;

  if (optionalOutTrace) {
    *optionalOutTrace << "searching for " << std::hex << formId << std::endl;
  }

  auto it = forms.find(formId);
  if (it == forms.end()) {
    if (formId < 0xff000000) {
      if (LoadForm(formId, optionalOutTrace)) {
        it = forms.find(formId);
        if (it != forms.end()) {
          if (optionalOutTrace) {
            *optionalOutTrace << "found after successful LoadForm" << std::hex
                              << formId << std::endl;
          }
          return it->second;
        }
        if (optionalOutTrace) {
          *optionalOutTrace << "not found after successful LoadForm"
                            << std::hex << formId << std::endl;
        }
        return kNullForm;
      } else {
        if (optionalOutTrace) {
          *optionalOutTrace << "LoadForm returned false " << std::hex << formId
                            << std::endl;
        }
      }
    }
    if (optionalOutTrace) {
      *optionalOutTrace << "not found " << std::hex << formId << std::endl;
    }
    return kNullForm;
  }

  if (optionalOutTrace) {
    *optionalOutTrace << "found " << std::hex << formId << std::endl;
  }
  return it->second;
}

bool WorldState::AttachEspmRecord(const espm::CombineBrowser& br,
                                  const espm::RecordHeader* record,
                                  const espm::IdMapping& mapping,
                                  std::stringstream* optionalOutTrace)
{
  auto& cache = GetEspmCache();
  // this place is a hotpath.
  // we want to use reinterpret_cast<> instead of espm::Convert<>
  // in order to reduce amount of generated assembly code
  auto* refr = reinterpret_cast<const espm::REFR*>(record);
  auto data = refr->GetData(cache);

  uint32_t baseId = espm::utils::GetMappedId(data.baseId, mapping);
  espm::LookupResult base = br.LookupById(baseId);
  if (!base.rec) {
    logger->info("baseId {} {}", baseId, static_cast<const void*>(base.rec));
    if (optionalOutTrace) {
      *optionalOutTrace << fmt::format(
        "AttachEspmRecord - base record not found {:x} \n", baseId);
    }
    return false;
  }

  espm::Type t = base.rec->GetType();
  bool isNpc = t == "NPC_";
  bool isFurniture = t == "FURN";
  bool isActivator = t == "ACTI";
  bool isDoor = t == "DOOR";
  bool isContainer = t == "CONT";
  bool isFlor = t == "FLOR" &&
    reinterpret_cast<const espm::FLOR*>(base.rec)->GetData(cache).resultItem;
  bool isTree = t == "TREE" &&
    reinterpret_cast<const espm::TREE*>(base.rec)->GetData(cache).resultItem;

  if (!isNpc && !isFurniture && !isActivator && !espm::utils::IsItem(t) &&
      !isDoor && !isContainer && !isFlor && !isTree) {
    if (optionalOutTrace) {
      *optionalOutTrace << fmt::format(
        "AttachEspmRecord - the server skips base type {} \n", t.ToString());
    }
    return false;
  }

  bool startsDead = false;
  if (isNpc) {
    auto* achr = reinterpret_cast<const espm::ACHR*>(record);
    startsDead = achr->StartsDead();
    if (startsDead) {
      if (optionalOutTrace) {
        *optionalOutTrace << fmt::format(
          "AttachEspmRecord - the server skips dead actors\n");
      }
      return false; // TODO: Load dead references
    }
  }

  // TODO: Load disabled references
  enum
  {
    InitiallyDisabled = 0x800,
    DeletedRecord = 0x20
  };

  if (refr->GetFlags() & InitiallyDisabled) {
    if (optionalOutTrace) {
      *optionalOutTrace << fmt::format(
        "AttachEspmRecord - the server skips initially disabled references\n");
    }
    return false;
  }

  if (refr->GetFlags() & DeletedRecord) {
    if (optionalOutTrace) {
      *optionalOutTrace << fmt::format(
        "AttachEspmRecord - the server skips deleted references\n");
    }
    return false;
  }

  if (!npcEnabled && isNpc) {
    if (optionalOutTrace) {
      *optionalOutTrace << fmt::format(
        "AttachEspmRecord - the server skips npcs: npcEnabled = false\n");
    }
    return false;
  }

  uint32_t formId = espm::utils::GetMappedId(record->GetId(), mapping);

  if (isNpc) {
    if (NpcSourceFilesOverriden() && !IsNpcAllowed(formId)) {
      spdlog::trace("Skip NPC loading, it is not allowed. refrId {:#x}",
                    formId);
      if (optionalOutTrace) {
        *optionalOutTrace
          << fmt::format("Skip NPC loading, it is not allowed. refrId {:#x}",
                         formId)
          << std::endl;
      }
      return false;
    }
    auto npcData =
      reinterpret_cast<const espm::NPC_*>(base.rec)->GetData(cache);

    if (npcData.isEssential || npcData.isProtected || npcData.isUnique) {
      if (optionalOutTrace) {
        *optionalOutTrace << fmt::format("Skip NPC due to its flags")
                          << std::endl;
      }
      return false;
    }

    enum class ListType : uint32_t
    {
      CrimeFactionsList = 0x26953
    };

    auto factionBaseId = static_cast<std::underlying_type_t<ListType>>(
      ListType::CrimeFactionsList);
    espm::LookupResult res = br.LookupById(factionBaseId);
    auto* formList = reinterpret_cast<const espm::FLST*>(res.rec);
    std::vector<uint32_t> factionFormIds = formList->GetData(cache).formIds;
    for (auto& formId : factionFormIds) {
      formId = res.ToGlobalId(formId);
    }

    for (auto fact : npcData.factions) {
      auto it = std::find(factionFormIds.begin(), factionFormIds.end(),
                          base.ToGlobalId(fact.formId));
      if (it != factionFormIds.end()) {
        logger->info("Skipping actor {:#x} because it's in faction {:#x}",
                     record->GetId(), *it);
        if (optionalOutTrace) {
          *optionalOutTrace << fmt::format("Skip NPC due to faction")
                            << std::endl;
        }
        return false;
      }
    }

    const int kPcLevel = 0; // Shouldn't mean much to races

    // May be not exact the same template chain as it would be in MpActor
    // but it's ok for this check
    std::vector<uint32_t> evaluateChainResult =
      LeveledListUtils::EvaluateTemplateChain(br, base, kPcLevel);
    std::vector<FormDesc> templateChain(evaluateChainResult.size());
    std::transform(evaluateChainResult.begin(), evaluateChainResult.end(),
                   templateChain.begin(), [&](uint32_t formId) {
                     return FormDesc::FromFormId(formId, this->espmFiles);
                   });

    uint32_t race = EvaluateTemplate<espm::NPC_::UseTraits>(
      this, baseId, templateChain,
      [&](const auto& npcLookupResult, const auto& npcData) {
        return npcLookupResult.ToGlobalId(npcData.race);
      });

    bool isBanned = std::find(bannedEspmCharacterRaceIds.begin(),
                              bannedEspmCharacterRaceIds.end(),
                              race) != bannedEspmCharacterRaceIds.end();
    if (isBanned) {
      logger->info("Skipping actor {:#x} because it has banned race {:#x}",
                   record->GetId(), race);
      if (optionalOutTrace) {
        *optionalOutTrace << fmt::format("Skip NPC due to banned race")
                          << std::endl;
      }
      return false;
    }
  }

  auto locationalData = data.loc;

  uint32_t worldOrCell =
    espm::utils::GetMappedId(GetWorldOrCell(br, record), mapping);
  if (!worldOrCell) {
    logger->error("Anomaly: refr without world/cell");
    if (optionalOutTrace) {
      *optionalOutTrace << fmt::format("Anomaly: refr without world/cell")
                        << std::endl;
    }
    return false;
  }

  if (isNpc && NpcSourceFilesOverriden()) {
    bool isInterior = false, isExterior = false,
         spawnInInterior = defaultSetting.spawnInInterior,
         spawnInExterior = defaultSetting.spawnInExterior;
    espm::LookupResult res = br.LookupById(worldOrCell);
    auto* cellRecord = espm::Convert<espm::CELL>(res.rec);
    if (cellRecord) {
      isInterior = true;
    }
    auto* worldRecord = espm::Convert<espm::WRLD>(res.rec);
    if (worldRecord) {
      isExterior = true;
    }
    uint32_t npcFileIdx = GetFileIdx(formId);
    if (npcFileIdx >= espmFiles.size()) {
      spdlog::error("NPC's idx is greater than espmFiles.size(). NPC's"
                    "formId "
                    "{:#x}, espmFiles size: {}",
                    formId, espmFiles.size());
      if (optionalOutTrace) {
        *optionalOutTrace
          << fmt::format("NPC's idx is greater than espmFiles.size(). NPC's"
                         "formId "
                         "{:#x}, espmFiles size: {}",
                         formId, espmFiles.size())
          << std::endl;
      }
      return false;
    }
    auto it = npcSettings.find(espmFiles[npcFileIdx]);
    if (it != npcSettings.end()) {
      spawnInInterior = it->second.spawnInInterior;
      spawnInExterior = it->second.spawnInExterior;
      spdlog::trace(
        "found npc setting spawnInInterior: {}, spawnInExterior: {}",
        spawnInInterior, spawnInExterior);
    } else {
      spdlog::trace("npc setting has not been found. Use default "
                    "spawnInInterior: {} and spawnInExterior: {}",
                    spawnInInterior, spawnInExterior);
    }

    if (spawnInInterior && isInterior || spawnInExterior && isExterior) {
    }

    if ((!spawnInInterior || !isInterior) &&
        (!spawnInExterior || !isExterior)) {
      spdlog::trace(
        "Unable to spawn npc because of "
        "rules applied in server settings: spawnInInterior={}, "
        "spawnInExterior={}, NPC location: exterior={}, interior={}",
        spawnInInterior, spawnInExterior, isExterior, isInterior);
      if (optionalOutTrace) {
        *optionalOutTrace
          << fmt::format(
               "Unable to spawn npc because of "
               "rules applied in server settings: spawnInInterior={}, "
               "spawnInExterior={}, NPC location: exterior={}, interior={}",
               spawnInInterior, spawnInExterior, isExterior, isInterior)
          << std::endl;
      }
      return false;
    }
  }

  if (!locationalData) {
    logger->error("Anomaly: refr without locationalData");
    if (optionalOutTrace) {
      *optionalOutTrace << fmt::format("Anomaly: refr without locationalData")
                        << std::endl;
    }
    return false;
  }

  std::optional<NiPoint3> primitiveBoundsDiv2;
  if (data.boundsDiv2) {
    primitiveBoundsDiv2 =
      NiPoint3(data.boundsDiv2[0], data.boundsDiv2[1], data.boundsDiv2[2]);
  }

  auto typeStr = t.ToString();
  std::unique_ptr<MpForm> form;
  LocationalData formLocationalData = {
    LocationalDataUtils::GetPos(locationalData),
    LocationalDataUtils::GetRot(locationalData),
    FormDesc::FromFormId(worldOrCell, espmFiles)
  };
  if (!isNpc) {
    form.reset(new MpObjectReference(formLocationalData,
                                     formCallbacksFactory(), baseId,
                                     typeStr.data(), primitiveBoundsDiv2));
  } else {
    form.reset(
      new MpActor(formLocationalData, formCallbacksFactory(), baseId));
  }
  AddForm(std::move(form), formId, true);

  // Do not TriggerFormInitEvent here, doing it later after changeForm apply

  if (optionalOutTrace) {
    *optionalOutTrace << fmt::format("AttachEspmRecord returned true")
                      << std::endl;
  }
  return true;
}

bool WorldState::LoadForm(uint32_t formId, std::stringstream* optionalOutTrace)
{
  auto& br = GetEspm().GetBrowser();

  auto lookupRes = br.LookupById(formId);

  if (!lookupRes.rec) {
    return false;
  }

  auto mapping = br.GetCombMapping(lookupRes.fileIdx);

  bool attached =
    AttachEspmRecord(br, lookupRes.rec, *mapping, optionalOutTrace);
  if (optionalOutTrace) {
    *optionalOutTrace << "AttachEspmRecord " << (attached ? "true" : "false")
                      << std::endl;
  }

  if (attached) {
    auto& refr = GetFormAt<MpObjectReference>(formId);
    auto it = pImpl->changeFormsForDeferredLoad.find(formId);
    if (it != pImpl->changeFormsForDeferredLoad.end()) {
      auto copy = it->second; // crashes without copying
      refr.ApplyChangeForm(copy);
    }

    refr.ForceSubscriptionsUpdate();
  }

  return attached;
}

void WorldState::TickReloot(const std::chrono::system_clock::time_point& now)
{
  for (auto& p : relootTimers) {
    auto& list = p.second;
    while (!list.empty() && list.begin()->second <= now) {
      uint32_t relootTargetId = list.begin()->first;
      auto relootTarget = std::dynamic_pointer_cast<MpObjectReference>(
        LookupFormById(relootTargetId));
      if (relootTarget) {
        relootTarget->DoReloot();
      }

      list.pop_front();
    }
  }
}

void WorldState::TickSaveStorage(const std::chrono::system_clock::time_point&)
{
  if (!pImpl->saveStorage) {
    return;
  }

  pImpl->saveStorage->Tick();

  auto& changes = pImpl->changes;
  if (!pImpl->saveStorageBusy && !changes.empty()) {
    pImpl->saveStorageBusy = true;
    std::vector<MpChangeForm> changeForms;
    changeForms.reserve(changes.size());
    for (auto [formId, changeForm] : changes) {
      changeForms.push_back(changeForm);
    }
    changes.clear();

    auto pImpl_ = pImpl;
    pImpl->saveStorage->Upsert(changeForms,
                               [pImpl_] { pImpl_->saveStorageBusy = false; });
  }
}

void WorldState::TickTimers(const std::chrono::system_clock::time_point&)
{
  timerEffects.TickTimers();
  timerRegular.TickTimers();
}

void WorldState::SendPapyrusEvent(MpForm* form, const char* eventName,
                                  const VarValue* arguments,
                                  size_t argumentsCount)
{
  std::vector<VarValue> args = { arguments, arguments + argumentsCount };

  if (spdlog::should_log(spdlog::level::trace)) {
    std::vector<std::string> argsStrings(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
      argsStrings[i] = args[i].ToString();
    }

    if (!strcmp(eventName, "OnTrigger")) {
      static std::once_flag g_once;
      std::call_once(g_once, [&] {
        spdlog::trace("WorldState::SendPapyrusEvent {:x} - {} [{}]",
                      form->GetFormId(), eventName,
                      fmt::join(argsStrings, ", "));
        spdlog::trace("WorldState::SendPapyrusEvent {:x} - Muting {} globally "
                      "to keep logs clear",
                      form->GetFormId(), eventName);
      });
    } else {
      spdlog::trace("WorldState::SendPapyrusEvent {:x} - {} [{}]",
                    form->GetFormId(), eventName,
                    fmt::join(argsStrings, ", "));
    }
  }

  VirtualMachine::OnEnter onEnter = [&](const StackIdHolder& holder) {
    pImpl->policy->BeforeSendPapyrusEvent(form, eventName, arguments,
                                          argumentsCount, holder.GetStackId());
  };
  auto& vm = GetPapyrusVm();
  return vm.SendEvent(form->ToGameObject(), eventName, args, onEnter);
}

const std::set<MpObjectReference*>& WorldState::GetReferencesAtPosition(
  uint32_t cellOrWorld, int16_t cellX, int16_t cellY)
{
  if (espm && !pImpl->chunkLoadingInProgress) {
    Viet::ScopedTask<bool> task([](bool& st) { st = false; },
                                pImpl->chunkLoadingInProgress);
    pImpl->chunkLoadingInProgress = true;

    auto& br = espm->GetBrowser();
    for (int16_t x = cellX - 1; x <= cellX + 1; ++x) {
      for (int16_t y = cellY - 1; y <= cellY + 1; ++y) {
        const bool loaded = grids[cellOrWorld].loadedChunks[x][y];
        if (!loaded) {
          for (size_t i = 0; i < espmFiles.size(); ++i) {
            auto combMapping = br.GetCombMapping(i);
            auto rawMapping = br.GetRawMapping(i);
            uint32_t mappedCellOrWorld =
              espm::utils::GetMappedId(cellOrWorld, *rawMapping);
            auto records = br.GetRecordsAtPos(mappedCellOrWorld, x, y);
            for (auto rec : *records[i]) {
              auto mappedId =
                espm::utils::GetMappedId(rec->GetId(), *combMapping);
              assert(mappedId < 0xff000000);
              LoadForm(mappedId);
            }
          }
          // Do not keep "loaded" reference here since LoadForm would
          // invalidate this reference
          grids[cellOrWorld].loadedChunks[x][y] = true;
        }
      }
    }
  }

  auto& neighbours =
    grids[cellOrWorld].grid->GetNeighboursByPosition(cellX, cellY);
  return neighbours;
}

MpForm* WorldState::LookupFormByIdx(int idx)
{
  if (formIdxManager) {
    if (idx >= 0 && idx < formByIdxUnreliable.size()) {
      auto form = formByIdxUnreliable[idx];
      if (auto formIndex = dynamic_cast<FormIndex*>(form)) {
        if (formIndex->GetIdx() == idx)
          return form;
      }
    }
  }
  return nullptr;
}

espm::Loader& WorldState::GetEspm() const
{
  if (!espm) {
    throw std::runtime_error("No espm attached");
  }
  return *espm;
}

bool WorldState::HasEspm() const
{
  return !!espm;
}

espm::CompressedFieldsCache& WorldState::GetEspmCache()
{
  if (!espmCache) {
    throw std::runtime_error("No espm cache found");
  }
  return *espmCache;
}

IScriptStorage* WorldState::GetScriptStorage() const
{
  return pImpl->scriptStorage.get();
}

struct LazyState
{
  std::shared_ptr<PexScript> pex;
  std::vector<uint8_t> pexBin;

  // With Papyrus hotreload enabled, this variable hold references to previous
  // versions of pex files. This prevents the invalidation of string/identifier
  // types of VarValue
  std::vector<std::shared_ptr<PexScript>> oldPexHolder;
};

PexScript::Lazy CreatePexScriptLazy(
  const CIString& required, std::shared_ptr<IScriptStorage> scriptStorage,
  std::shared_ptr<spdlog::logger> logger, bool enableHotReload)
{
  auto lazyState = std::make_shared<LazyState>();

  PexScript::Lazy lazy;
  lazy.source = required.data();
  lazy.fn = [lazyState, scriptStorage, required, logger, enableHotReload]() {
    if (enableHotReload) {
      auto requiredPex = scriptStorage->GetScriptPex(required.data());
      if (requiredPex != lazyState->pexBin) {
        lazyState->oldPexHolder.push_back(lazyState->pex);
        lazyState->pex.reset();
        lazyState->pexBin = requiredPex;
        logger->info("Papyrus script {} has been reloaded", required);
      }
    }

    if (!lazyState->pex) {
      auto requiredPex = scriptStorage->GetScriptPex(required.data());
      if (requiredPex.empty()) {
        throw std::runtime_error(
          "'" + std::string({ required.begin(), required.end() }) +
          "' is listed but failed to "
          "load from the storage");
      }
      auto pexStructure = Reader({ requiredPex }).GetSourceStructures();
      lazyState->pex = pexStructure[0];
    }
    return lazyState->pex;
  };

  return lazy;
}

VirtualMachine& WorldState::GetPapyrusVm()
{
  if (!pImpl->vm) {
    std::vector<PexScript::Lazy> pexStructures;
    std::vector<std::string> scriptNames;

    auto scriptStorage = pImpl->scriptStorage;
    if (!scriptStorage) {
      logger->error("Required scriptStorage to be non-null");
      pImpl->vm.reset(
        new VirtualMachine(std::vector<std::shared_ptr<PexScript>>()));
      return *pImpl->vm;
    }

    auto& scripts = scriptStorage->ListScripts(false);
    for (auto& required : scripts) {
      auto lazy = CreatePexScriptLazy(required, scriptStorage, this->logger,
                                      this->isPapyrusHotReloadEnabled);
      pexStructures.push_back(lazy);
    }

    if (!pexStructures.empty()) {
      pImpl->vm.reset(new VirtualMachine(pexStructures));

      pImpl->vm->SetMissingScriptHandler(
        [scriptStorage, this](std::string className) {
          std::optional<PexScript::Lazy> result;

          CIString classNameCi = { className.begin(), className.end() };
          if (scriptStorage->ListScripts(true).count(classNameCi)) {
            result =
              CreatePexScriptLazy(classNameCi, scriptStorage, this->logger,
                                  this->isPapyrusHotReloadEnabled);
          }
          return result;
        });

      pImpl->vm->SetExceptionHandler([this](const VmExceptionInfo& errorData) {
        std::string sourcePex = errorData.sourcePex;
        std::string what = errorData.what;
        std::string loggerMsg = sourcePex + ": " + what;
        bool methodNotFoundError =
          what.find("Method not found") != std::string::npos;
        if (methodNotFoundError) {
          logger->warn(loggerMsg);
        } else {
          logger->error(loggerMsg);
        }
      });

      pImpl->classes =
        PapyrusClassesFactory::CreateAndRegister(*pImpl->vm, pImpl->policy);
    }
  }

  return *pImpl->vm;
}

const std::set<uint32_t>& WorldState::GetActorsByProfileId(
  int32_t profileId) const
{
  static const std::set<uint32_t> kEmptySet;

  auto it = actorIdByProfileId.find(profileId);
  if (it == actorIdByProfileId.end()) {
    return kEmptySet;
  }
  return it->second;
}

const std::set<uint32_t>& WorldState::GetActorsByPrivateIndexedProperty(
  const std::string& privateIndexedPropertyMapKey) const
{
  static const std::set<uint32_t> kEmptySet;

  auto it = actorIdByPrivateIndexedProperty.find(privateIndexedPropertyMapKey);
  if (it == actorIdByPrivateIndexedProperty.end()) {
    return kEmptySet;
  }
  return it->second;
}

std::string WorldState::MakePrivateIndexedPropertyMapKey(
  const std::string& propertyName, const std::string& propertyValueStringified)
{
  return propertyName + '=' + propertyValueStringified;
}

uint32_t WorldState::GenerateFormId()
{
  while (LookupFormById(pImpl->nextId)) {
    ++pImpl->nextId;
  }
  return pImpl->nextId++;
}

void WorldState::SetRelootTime(std::string recordType,
                               std::chrono::system_clock::duration dur)
{
  pImpl->relootTimeForTypes[recordType] = dur;
}

std::optional<std::chrono::system_clock::duration> WorldState::GetRelootTime(
  std::string recordType) const
{
  auto it = pImpl->relootTimeForTypes.find(recordType);
  if (it == pImpl->relootTimeForTypes.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool WorldState::HasKeyword(uint32_t baseId, const char* keyword)
{
  auto lookupRes = GetEspm().GetBrowser().LookupById(baseId);

  if (!lookupRes.rec) {
    return false;
  }

  const auto keywordIds = lookupRes.rec->GetKeywordIds(GetEspmCache());

  for (auto keywordId : keywordIds) {
    auto keywordIdGlobal = lookupRes.ToGlobalId(keywordId);
    auto rec = GetEspm().GetBrowser().LookupById(keywordIdGlobal).rec;
    if (rec && !Utils::stricmp(rec->GetEditorId(GetEspmCache()), keyword)) {
      return true;
    }
  }

  return false;
}

bool WorldState::NpcSourceFilesOverriden() const noexcept
{
  return !npcSettings.empty() || defaultSetting.overriden;
}

bool WorldState::IsNpcAllowed(uint32_t refrId) const noexcept
{
  if (npcSettings.empty() && defaultSetting.overriden) {
    return true;
  }
  uint32_t npcFileIdx = GetFileIdx(refrId);
  for (const auto& [fileName, _] : npcSettings) {
    auto it = std::find(espmFiles.begin(), espmFiles.end(), fileName);
    if (it == espmFiles.end()) {
      return false;
    }
    auto idx = std::distance(espmFiles.begin(), it);
    if (npcFileIdx == idx) {
      return true;
    }
  }
  return false;
}

uint32_t WorldState::GetFileIdx(uint32_t formId) const noexcept
{
  return formId >> 24;
}

void WorldState::SetNpcSettings(
  std::unordered_map<std::string, NpcSettingsEntry>&& settings)
{
  npcSettings = settings;
}

bool WorldState::RemoveTimer(uint32_t timerId)
{
  return timerRegular.RemoveTimer(timerId);
}

bool WorldState::RemoveEffectTimer(uint32_t timerId)
{
  return timerEffects.RemoveTimer(timerId);
}

void WorldState::SetForbiddenRelootTypes(const std::set<std::string>& types)
{
  pImpl->forbiddenRelootTypes = types;
}

void WorldState::SetEnableConsoleCommandsForAllSetting(bool enable)
{
  enableConsoleCommandsForAll = enable;
}

bool WorldState::IsRelootForbidden(std::string type) const noexcept
{
  return pImpl->forbiddenRelootTypes.find(type) !=
    pImpl->forbiddenRelootTypes.end();
}

bool WorldState::HasEspmFile(std::string_view filename) const noexcept
{
  return std::find(espmFiles.begin(), espmFiles.end(), filename) !=
    espmFiles.end();
}
