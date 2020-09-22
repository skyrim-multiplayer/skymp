#include "MpActor.h"
#include "ChangeFormGuard.h"
#include "WorldState.h"
#include <NiPoint3.h>

struct MpActor::Impl : public ChangeFormGuard<MpChangeFormACHR>
{
  Impl(MpChangeFormACHR changeForm_, MpObjectReference* self_)
    : ChangeFormGuard(changeForm_, self_)
  {
  }
};

MpActor::MpActor(const LocationalData& locationalData_,
                 const FormCallbacks& callbacks_, uint32_t optBaseId)
  : MpObjectReference(locationalData_, callbacks_, optBaseId, "NPC_")
{
  pImpl.reset(new Impl{ MpChangeFormACHR(), this });
}

void MpActor::SetRaceMenuOpen(bool isOpen)
{
  pImpl->EditChangeForm(
    [&](MpChangeFormACHR& changeForm) { changeForm.isRaceMenuOpen = isOpen; });
}

void MpActor::SetLook(const Look* newLook)
{
  pImpl->EditChangeForm([&](MpChangeFormACHR& changeForm) {
    if (newLook)
      changeForm.look = *newLook;
    else
      changeForm.look.reset();
  });
}

void MpActor::SetEquipment(const std::string& jsonString)
{
  pImpl->EditChangeForm([&](MpChangeFormACHR& changeForm) {
    if (jsonString.size() > 0) {
      simdjson::dom::parser p;
      auto element = p.parse(jsonString).value();
      changeForm.equipment = Equipment::FromJson(element);
    } else
      changeForm.equipment.reset();
  });
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
  static_cast<MpChangeFormACHR&>(res) = pImpl->ChangeForm();
  res.recType = MpChangeForm::ACHR;
  return res;
}

void MpActor::ApplyChangeForm(const MpChangeForm& changeForm)
{
  if (changeForm.recType != MpChangeForm::ACHR) {
    throw std::runtime_error(
      "Expected record type to be ACHR, but found REFR");
  }
  MpObjectReference::ApplyChangeForm(changeForm);
  pImpl->EditChangeForm(
    [&](MpChangeFormACHR& changeForm) {
      changeForm = static_cast<const MpChangeFormACHR&>(changeForm);
    },
    Impl::Mode::NoRequestSave);
}

const bool& MpActor::IsRaceMenuOpen() const
{
  return pImpl->ChangeForm().isRaceMenuOpen;
}

const Look* MpActor::GetLook() const
{
  auto& changeForm = pImpl->ChangeForm();
  if (changeForm.look)
    return &*changeForm.look;
  return nullptr;
}

std::string MpActor::GetLookAsJson()
{
  if (GetLook())
    return GetLook()->ToJson();
  return "";
}

std::string MpActor::GetEquipmentAsJson()
{
  if (pImpl->ChangeForm().equipment)
    return pImpl->ChangeForm().equipment->ToJson().dump();
  else
    return "";
};

void MpActor::UnsubscribeFromAll()
{
  auto emittersCopy = GetEmitters();
  for (auto emitter : emittersCopy)
    if (emitter != this)
      Unsubscribe(emitter, this);
}

void MpActor::BeforeDestroy()
{
  for (auto& sink : destroyEventSinks)
    sink->BeforeDestroy(*this);

  MpObjectReference::BeforeDestroy();

  UnsubscribeFromAll();
}