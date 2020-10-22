#include "WorldState.h"
#include "ISaveStorage.h"
#include "MpActor.h"
#include "MpChangeForms.h"
#include "MpObjectReference.h"
#include "PapyrusGame.h"
#include "PapyrusObjectReference.h"
#include "Reader.h"
#include "ScriptStorage.h"
#include <unordered_map>

struct WorldState::Impl
{
  std::unordered_map<uint32_t, MpChangeForm> changes;
  std::shared_ptr<ISaveStorage> saveStorage;
  std::shared_ptr<IScriptStorage> scriptStorage;
  bool saveStorageBusy = false;
  std::shared_ptr<VirtualMachine> vm;
  uint32_t nextId = 0xff000000;
};

WorldState::WorldState()
{
  pImpl.reset(new Impl);
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
  form->Init(this, formId);

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
  // Tick Reloot
  auto now = std::chrono::system_clock::now();
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

const std::shared_ptr<MpForm>& WorldState::LookupFormById(uint32_t formId)
{
  auto it = forms.find(formId);
  if (it == forms.end()) {
    static const std::shared_ptr<MpForm> g_null;
    return g_null;
  }
  return it->second;
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

VirtualMachine& WorldState::GetPapyrusVm()
{
  if (!pImpl->vm) {
    std::vector<std::shared_ptr<PexScript>> pexStructures;
    std::vector<std::string> scriptNames;

    std::vector<std::vector<uint8_t>> pexVec;

    auto scriptStorage = GetScriptStorage();
    if (!scriptStorage)
      throw std::runtime_error("Required scriptStorage to be non-null");

    auto& scripts = scriptStorage->ListScripts();
    for (auto& required : scripts) {
      auto requiredPex = scriptStorage->GetScriptPex(required.data());
      if (requiredPex.empty())
        throw std::runtime_error(
          "'" + std::string(required) +
          "' is listed but failed to load from the storage");
      pexVec.push_back(requiredPex);
    }

    auto pexStructure = Reader(pexVec).GetSourceStructures();
    for (auto& v : pexStructure)
      pexStructures.push_back(v);

    if (!pexStructures.empty()) {
      pImpl->vm.reset(new VirtualMachine(pexStructures));
      PapyrusObjectReference::Register(*pImpl->vm);
      PapyrusGame::Register(*pImpl->vm);
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