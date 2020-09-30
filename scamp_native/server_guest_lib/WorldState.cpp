#include "WorldState.h"
#include "MpActor.h"

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
}

void WorldState::AddForm(std::unique_ptr<MpForm> form, uint32_t formId,
                         bool skipChecks)
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

  forms.insert({ formId, std::move(form) });
}

void WorldState::TickTimers()
{
  auto now = std::chrono::steady_clock::now();

  for (auto& p : relootTimers) {
    auto& list = p.second;
    while (!list.empty() && list.begin()->second <= now) {
      uint32_t relootTargetId = list.begin()->first;
      auto relootTarget = std::dynamic_pointer_cast<MpObjectReference>(
        LookupFormById(relootTargetId));
      if (relootTarget) {
        relootTarget->SetOpen(false);
        relootTarget->SetHarvested(false);
        relootTarget->RelootContainer();
      }

      list.pop_front();
    }
  }
}

void WorldState::RequestReloot(MpObjectReference& ref)
{
  auto& list = relootTimers[ref.GetRelootTime()];
  list.push_back({ ref.GetFormId(),
                   std::chrono::steady_clock::now() + ref.GetRelootTime() });
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