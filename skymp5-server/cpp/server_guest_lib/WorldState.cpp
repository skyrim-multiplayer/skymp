#include "WorldState.h"
#include "FormCallbacks.h"
#include "HeuristicPolicy.h"
#include "ISaveStorage.h"
#include "MpActor.h"
#include "MpChangeForms.h"
#include "MpFormGameObject.h"
#include "MpObjectReference.h"
#include "PapyrusActor.h"
#include "PapyrusDebug.h"
#include "PapyrusEffectShader.h"
#include "PapyrusForm.h"
#include "PapyrusFormList.h"
#include "PapyrusGame.h"
#include "PapyrusKeyword.h"
#include "PapyrusMessage.h"
#include "PapyrusObjectReference.h"
#include "PapyrusSkymp.h"
#include "PapyrusUtility.h"
#include "ScopedTask.h"
#include "ScriptStorage.h"
#include "Timer.h"
#include "libespm/GroupUtils.h"
#include "papyrus-vm/Reader.h"
#include <algorithm>
#include <deque>
#include <unordered_map>

namespace {
inline const NiPoint3& GetPos(const espm::REFR::LocationalData* locationalData)
{
  return *reinterpret_cast<const NiPoint3*>(locationalData->pos);
}

inline NiPoint3 GetRot(const espm::REFR::LocationalData* locationalData)
{
  static const auto g_pi = std::acos(-1.f);
  return { locationalData->rotRadians[0] / g_pi * 180.f,
           locationalData->rotRadians[1] / g_pi * 180.f,
           locationalData->rotRadians[2] / g_pi * 180.f };
}
}

struct WorldState::Impl
{
  std::unordered_map<uint32_t, MpChangeForm> changes;
  std::shared_ptr<ISaveStorage> saveStorage;
  std::shared_ptr<IScriptStorage> scriptStorage;
  bool saveStorageBusy = false;
  std::shared_ptr<VirtualMachine> vm;
  uint32_t nextId = 0xff000000;
  std::shared_ptr<HeuristicPolicy> policy;
  std::unordered_map<uint32_t, MpChangeForm> changeFormsForDeferredLoad;
  bool chunkLoadingInProgress = false;
  bool formLoadingInProgress = false;
  std::map<std::string, std::chrono::system_clock::duration>
    relootTimeForTypes;
  std::vector<std::unique_ptr<IPapyrusClassBase>> classes;
  Viet::Timer timer;
};

WorldState::WorldState()
{
  logger.reset(new spdlog::logger("empty logger"));

  pImpl.reset(new Impl);
  pImpl->policy.reset(new HeuristicPolicy(logger, this));
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
      std::stringstream ss;
      ss << std::hex << "Unable to find record " << baseId;
      throw std::runtime_error(ss.str());
    }
    baseType = rec->GetType().ToString();
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

void WorldState::RegisterForSingleUpdate(const VarValue& self, float seconds)
{
  SetTimer(seconds).Then([self](Viet::Void) {
    if (auto form = GetFormPtr<MpForm>(self))
      form->Update();
  });
}

Viet::Promise<Viet::Void> WorldState::SetTimer(float seconds)
{
  return pImpl->timer.SetTimer(seconds);
}

const std::shared_ptr<MpForm>& WorldState::LookupFormById(uint32_t formId)
{
  static const std::shared_ptr<MpForm> kNullForm;

  auto it = forms.find(formId);
  if (it == forms.end()) {
    if (formId < 0xff000000) {
      if (LoadForm(formId)) {
        it = forms.find(formId);
        return it == forms.end() ? kNullForm : it->second;
      }
    }
    return kNullForm;
  }
  return it->second;
}

bool WorldState::AttachEspmRecord(const espm::CombineBrowser& br,
                                  espm::RecordHeader* record,
                                  const espm::IdMapping& mapping)
{
  auto& cache = GetEspmCache();
  auto refr = reinterpret_cast<espm::REFR*>(record);
  auto data = refr->GetData(cache);

  auto baseId = espm::GetMappedId(data.baseId, mapping);
  auto base = br.LookupById(baseId);
  if (!base.rec) {
    logger->info("baseId {} {}", baseId, static_cast<void*>(base.rec));
    return false;
  }

  espm::Type t = base.rec->GetType();
  if (t != "NPC_" && t != "FURN" && t != "ACTI" && !espm::IsItem(t) &&
      t != "DOOR" && t != "CONT" &&
      (t != "FLOR" ||
       !reinterpret_cast<espm::FLOR*>(base.rec)->GetData(cache).resultItem) &&
      (t != "TREE" ||
       !reinterpret_cast<espm::TREE*>(base.rec)->GetData(cache).resultItem))
    return false;

  // TODO: Load disabled references
  enum
  {
    InitiallyDisabled = 0x800
  };
  if (refr->GetFlags() & InitiallyDisabled)
    return false;

  if (t == "NPC_") {
    return false;
  }

  auto formId = espm::GetMappedId(record->GetId(), mapping);
  auto locationalData = data.loc;

  uint32_t worldOrCell =
    espm::GetMappedId(GetWorldOrCell(br, record), mapping);
  if (!worldOrCell) {
    logger->error("Anomally: refr without world/cell");
    return false;
  }

  // This function dosen't use LookupFormById to prevent recursion
  auto existing = forms.find(formId);

  if (existing != forms.end()) {
    auto existingAsRefr =
      reinterpret_cast<MpObjectReference*>(existing->second.get());

    if (locationalData) {
      existingAsRefr->SetPosAndAngleSilent(GetPos(locationalData),
                                           GetRot(locationalData));

      assert(existingAsRefr->GetPos() == NiPoint3(GetPos(locationalData)));
    }

  } else {
    if (!locationalData) {
      logger->error("Anomally: refr without locationalData");
      return false;
    }

    std::optional<NiPoint3> primitiveBoundsDiv2;
    if (data.boundsDiv2)
      primitiveBoundsDiv2 =
        NiPoint3(data.boundsDiv2[0], data.boundsDiv2[1], data.boundsDiv2[2]);

    auto typeStr = t.ToString();
    std::unique_ptr<MpForm> form;
    LocationalData formLocationalData = {
      GetPos(locationalData), GetRot(locationalData),
      FormDesc::FromFormId(worldOrCell, espmFiles)
    };
    if (t != "NPC_") {
      form.reset(new MpObjectReference(formLocationalData,
                                       formCallbacksFactory(), baseId,
                                       typeStr.data(), primitiveBoundsDiv2));
    } else {
      form.reset(
        new MpActor(formLocationalData, formCallbacksFactory(), baseId));
    }
    AddForm(std::move(form), formId, true);
    // Do not TriggerFormInitEvent here, doing it later after changeForm apply
  }

  return true;
}

bool WorldState::LoadForm(uint32_t formId)
{
  bool atLeastOneLoaded = false;
  auto& br = GetEspm().GetBrowser();
  auto lookupResults = br.LookupByIdAll(formId);
  for (auto& lookupRes : lookupResults) {
    auto mapping = br.GetCombMapping(lookupRes.fileIdx);
    if (AttachEspmRecord(br, lookupRes.rec, *mapping)) {
      atLeastOneLoaded = true;
    }
  }

  if (atLeastOneLoaded) {
    auto& refr = GetFormAt<MpObjectReference>(formId);
    auto it = pImpl->changeFormsForDeferredLoad.find(formId);
    if (it != pImpl->changeFormsForDeferredLoad.end()) {
      auto copy = it->second; // crashes without copying
      refr.ApplyChangeForm(copy);
    }

    refr.ForceSubscriptionsUpdate();
  }

  return atLeastOneLoaded;
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
  pImpl->timer.TickTimers();
}

void WorldState::SendPapyrusEvent(MpForm* form, const char* eventName,
                                  const VarValue* arguments,
                                  size_t argumentsCount)
{
  VirtualMachine::OnEnter onEnter = [&](const StackIdHolder& holder) {
    pImpl->policy->BeforeSendPapyrusEvent(form, eventName, arguments,
                                          argumentsCount, holder.GetStackId());
  };
  auto& vm = GetPapyrusVm();
  std::vector<VarValue> args = { arguments, arguments + argumentsCount };
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
              espm::GetMappedId(cellOrWorld, *rawMapping);
            auto records = br.GetRecordsAtPos(mappedCellOrWorld, x, y);
            for (auto rec : *records[i]) {
              auto mappedId = espm::GetMappedId(rec->GetId(), *combMapping);
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

      pImpl->classes.emplace_back(std::make_unique<PapyrusObjectReference>());
      pImpl->classes.emplace_back(std::make_unique<PapyrusGame>());
      pImpl->classes.emplace_back(std::make_unique<PapyrusForm>());
      pImpl->classes.emplace_back(std::make_unique<PapyrusMessage>());
      pImpl->classes.emplace_back(std::make_unique<PapyrusFormList>());
      pImpl->classes.emplace_back(std::make_unique<PapyrusDebug>());
      pImpl->classes.emplace_back(std::make_unique<PapyrusActor>());
      pImpl->classes.emplace_back(std::make_unique<PapyrusSkymp>());
      pImpl->classes.emplace_back(std::make_unique<PapyrusUtility>());
      pImpl->classes.emplace_back(std::make_unique<PapyrusEffectShader>());
      pImpl->classes.emplace_back(std::make_unique<PapyrusKeyword>());
      for (auto& cl : pImpl->classes) {
        cl->Register(*pImpl->vm, pImpl->policy, this);
      }
    }
  }

  return *pImpl->vm;
}

const std::set<uint32_t>& WorldState::GetActorsByProfileId(
  int32_t profileId) const
{
  static const std::set<uint32_t> g_emptySet;

  auto it = actorIdByProfileId.find(profileId);
  if (it == actorIdByProfileId.end())
    return g_emptySet;
  return it->second;
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
