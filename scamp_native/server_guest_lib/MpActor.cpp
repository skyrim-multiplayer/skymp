#include "MpActor.h"
#include "ChangeFormGuard.h"
#include "WorldState.h"
#include <NiPoint3.h>

struct MpActor::Impl : public ChangeFormGuard<MpChangeForm>
{
  Impl(MpChangeForm changeForm_, MpObjectReference* self_)
    : ChangeFormGuard(changeForm_, self_)
  {
  }

  std::map<uint32_t, std::function<void(nlohmann::json)>> snippetCallbacks;
  uint32_t snippetIndex = 0;
};

MpActor::MpActor(const LocationalData& locationalData_,
                 const FormCallbacks& callbacks_, uint32_t optBaseId)
  : MpObjectReference(locationalData_, callbacks_, optBaseId, "NPC_")
{
  pImpl.reset(new Impl{ MpChangeForm(), this });
}

void MpActor::SetRaceMenuOpen(bool isOpen)
{
  pImpl->EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.isRaceMenuOpen = isOpen; });
}

void MpActor::SetLook(const Look* newLook)
{
  pImpl->EditChangeForm([&](MpChangeForm& changeForm) {
    if (newLook)
      changeForm.lookDump = newLook->ToJson();
    else
      changeForm.lookDump.clear();
  });
}

void MpActor::SetEquipment(const std::string& jsonString)
{
  pImpl->EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.equipmentDump = jsonString; });
}

void MpActor::VisitProperties(const PropertiesVisitor& visitor,
                              VisitPropertiesMode mode)
{
  MpObjectReference::VisitProperties(visitor, mode);
  if (mode == VisitPropertiesMode::All && IsRaceMenuOpen())
    visitor("isRaceMenuOpen", "true");
}

void MpActor::SendToUser(const void* data, size_t size, bool reliable)
{
  if (callbacks->sendToUser)
    callbacks->sendToUser(this, data, size, reliable);
  else
    throw std::runtime_error("sendToUser is nullptr");
}

void MpActor::AddEventSink(std::shared_ptr<DestroyEventSink> sink)
{
  destroyEventSinks.insert(sink);
}

void MpActor::RemoveEventSink(std::shared_ptr<DestroyEventSink> sink)
{
  destroyEventSinks.erase(sink);
}

MpChangeForm MpActor::GetChangeForm() const
{
  auto res = MpObjectReference::GetChangeForm();
  auto& achr = pImpl->ChangeForm();
  res.lookDump = achr.lookDump;
  res.isRaceMenuOpen = achr.isRaceMenuOpen;
  res.equipmentDump = achr.equipmentDump;

  res.recType = MpChangeForm::ACHR;
  return res;
}

void MpActor::ApplyChangeForm(const MpChangeForm& newChangeForm)
{
  if (newChangeForm.recType != MpChangeForm::ACHR) {
    throw std::runtime_error(
      "Expected record type to be ACHR, but found REFR");
  }
  MpObjectReference::ApplyChangeForm(newChangeForm);
  pImpl->EditChangeForm(
    [&](MpChangeForm& cf) {
      cf = static_cast<const MpChangeForm&>(newChangeForm);
    },
    Impl::Mode::NoRequestSave);
}

uint32_t MpActor::NextSnippetIndex(
  std::function<void(nlohmann::json)> callback)
{
  auto res = pImpl->snippetIndex++;
  if (callback)
    pImpl->snippetCallbacks[res] = callback;
  return res;
}

const bool& MpActor::IsRaceMenuOpen() const
{
  return pImpl->ChangeForm().isRaceMenuOpen;
}

std::unique_ptr<const Look> MpActor::GetLook() const
{
  auto& changeForm = pImpl->ChangeForm();
  if (changeForm.lookDump.size() > 0) {
    simdjson::dom::parser p;
    auto doc = p.parse(changeForm.lookDump).value();

    std::unique_ptr<const Look> res;
    res.reset(new Look(Look::FromJson(doc)));
    return res;
  }
  return nullptr;
}

const std::string& MpActor::GetLookAsJson()
{
  return pImpl->ChangeForm().lookDump;
}

const std::string& MpActor::GetEquipmentAsJson()
{
  return pImpl->ChangeForm().equipmentDump;
};

void MpActor::BeforeDestroy()
{
  for (auto& sink : destroyEventSinks)
    sink->BeforeDestroy(*this);

  MpObjectReference::BeforeDestroy();

  UnsubscribeFromAll();
}