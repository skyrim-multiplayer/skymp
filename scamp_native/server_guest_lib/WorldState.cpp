#include "WorldState.h"
#include "HeuristicPolicy.h"
#include "ISaveStorage.h"
#include "MpActor.h"
#include "MpChangeForms.h"
#include "MpFormGameObject.h"
#include "MpObjectReference.h"
#include "PapyrusActor.h"
#include "PapyrusDebug.h"
#include "PapyrusForm.h"
#include "PapyrusFormList.h"
#include "PapyrusGame.h"
#include "PapyrusMessage.h"
#include "PapyrusObjectReference.h"
#include "PapyrusSkymp.h"
#include "PapyrusUtility.h"
#include "Reader.h"
#include "ScriptStorage.h"
#include <algorithm>
#include <deque>
#include <unordered_map>

namespace {
struct TimerEntry
{
  Viet::Promise<Viet::Void> promise;
  std::chrono::system_clock::time_point finish;
};
}

struct WorldState::Impl
{
  std::unordered_map<uint32_t, MpChangeForm> changes;
  std::shared_ptr<ISaveStorage> saveStorage;
  std::shared_ptr<IScriptStorage> scriptStorage;
  bool saveStorageBusy = false;
  std::shared_ptr<VirtualMachine> vm;
  uint32_t nextId = 0xff000000;
  std::deque<TimerEntry> timers;
  std::shared_ptr<HeuristicPolicy> policy;
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

void WorldState::AttachEspm(espm::Loader* espm_)
{
  espm = espm_;
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

void WorldState::TickTimers()
{
  const auto now = std::chrono::system_clock::now();

  // Tick Reloot
  for (auto& p : relootTimers) {
    auto& list = p.second;
    while (!list.empty() && list.begin()->second <= now) {
      uint32_t relootTargetId = list.begin()->first;
      auto relootTarget = std::dynamic_pointer_cast<MpObjectReference>(
        LookupFormById(relootTargetId));
      if (relootTarget)
        relootTarget->DoReloot();

      list.pop_front();
    }
  }

  // Tick Save Storage
  if (pImpl->saveStorage) {
    pImpl->saveStorage->Tick();

    if (!pImpl->saveStorageBusy && !pImpl->changes.empty()) {
      pImpl->saveStorageBusy = true;
      std::vector<MpChangeForm> changeForms;
      changeForms.reserve(pImpl->changes.size());
      for (auto [formId, changeForm] : pImpl->changes)
        changeForms.push_back(changeForm);
      pImpl->changes.clear();

      auto pImpl_ = pImpl;
      pImpl->saveStorage->Upsert(
        changeForms, [pImpl_] { pImpl_->saveStorageBusy = false; });
    }
  }

  // Tick RegisterForSingleUpdate
  while (!pImpl->timers.empty() && now >= pImpl->timers.front().finish) {
    auto front = std::move(pImpl->timers.front());
    pImpl->timers.pop_front();
    front.promise.Resolve(Viet::Void());
  }
}

void WorldState::LoadChangeForm(const MpChangeForm& changeForm,
                                const FormCallbacks& callbacks)
{
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

  if (formId < 0xff000000)
    return GetFormAt<MpObjectReference>(formId).ApplyChangeForm(changeForm);

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
}

void WorldState::RequestReloot(MpObjectReference& ref)
{
  auto& list = relootTimers[ref.GetRelootTime()];
  list.push_back({ ref.GetFormId(),
                   std::chrono::system_clock::now() + ref.GetRelootTime() });
}

void WorldState::RequestSave(MpObjectReference& ref)
{
  pImpl->changes[ref.GetFormId()] = ref.GetChangeForm();
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
  Viet::Promise<Viet::Void> promise;

  auto finish = std::chrono::system_clock::now() +
    std::chrono::milliseconds(static_cast<int>(seconds * 1000));

  bool sortRequired = false;

  if (!pImpl->timers.empty() && finish > pImpl->timers.front().finish) {
    sortRequired = true;
  }

  pImpl->timers.push_front({ promise, finish });

  if (sortRequired) {
    std::sort(pImpl->timers.begin(), pImpl->timers.end(),
              [](const TimerEntry& lhs, const TimerEntry& rhs) {
                return lhs.finish < rhs.finish;
              });
  }

  return promise;
}

const std::shared_ptr<MpForm>& WorldState::LookupFormById(uint32_t formId)
{
  auto it = forms.find(formId);
  if (it == forms.end()) {
    static const std::shared_ptr<MpForm> g_null;
    return g_null;
  }
  return it->second;
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

espm::Loader& WorldState::GetEspm() const
{
  if (!espm)
    throw std::runtime_error("No espm attached");
  return *espm;
}

espm::CompressedFieldsCache& WorldState::GetEspmCache()
{
  if (!espmCache)
    throw std::runtime_error("No espm cache found");
  return *espmCache;
}

IScriptStorage* WorldState::GetScriptStorage() const
{
  return pImpl->scriptStorage.get();
}

struct LazyState
{
  std::shared_ptr<PexScript> pex;
};

VirtualMachine& WorldState::GetPapyrusVm()
{
  if (!pImpl->vm) {
    std::vector<PexScript::Lazy> pexStructures;
    std::vector<std::string> scriptNames;

    auto scriptStorage = pImpl->scriptStorage;
    if (!scriptStorage) {
      logger->error("Required scriptStorage to be non-null");
      pImpl->vm.reset(new VirtualMachine(std::vector<PexScript::Ptr>()));
      return *pImpl->vm;
    }

    auto& scripts = scriptStorage->ListScripts();
    for (auto& required : scripts) {
      std::shared_ptr<LazyState> lazyState(new LazyState);
      PexScript::Lazy lazy;
      lazy.source = required.data();
      lazy.fn = [lazyState, scriptStorage, required]() {
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
      if (lazyMode == LazyMode::Disabled) {
        (void)lazy.fn();
      }
      pexStructures.push_back(lazy);
    }

    if (!pexStructures.empty()) {
      pImpl->vm.reset(new VirtualMachine(pexStructures));
      pImpl->vm->SetExceptionHandler(
        [&](const std::string& error) { logger->error("{}", error); });

      std::vector<IPapyrusClassBase*> classes;
      classes.emplace_back(new PapyrusObjectReference);
      classes.emplace_back(new PapyrusGame);
      classes.emplace_back(new PapyrusForm);
      classes.emplace_back(new PapyrusMessage);
      classes.emplace_back(new PapyrusFormList);
      classes.emplace_back(new PapyrusDebug);
      classes.emplace_back(new PapyrusActor);
      classes.emplace_back(new PapyrusSkymp);
      classes.emplace_back(new PapyrusUtility);
      for (auto cl : classes)
        cl->Register(*pImpl->vm, pImpl->policy);
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