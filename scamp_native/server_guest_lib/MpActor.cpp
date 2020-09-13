#include "MpActor.h"
#include "WorldState.h"
#include <NiPoint3.h>

struct MpActor::Impl
{
  MpChangeFormACHR changeForm;
};

MpActor::MpActor(const LocationalData& locationalData_,
                 const SubscribeCallback& onSubscribe_,
                 const SubscribeCallback& onUnsubscribe_,
                 const SendToUserFn& sendToUser_)
  : MpObjectReference(locationalData_, onSubscribe_, onUnsubscribe_,
                      nullBaseId, "ACHR")
  , sendToUser(sendToUser_)
{
  pImpl.reset(new Impl);
}

void MpActor::SetRaceMenuOpen(bool isOpen)
{
  pImpl->changeForm.isRaceMenuOpen = isOpen;
}

void MpActor::SetLook(const Look* newLook)
{
  if (newLook) {
    pImpl->changeForm.look = *newLook;
  } else {
    pImpl->changeForm.look.reset();
  }
}

void MpActor::SetEquipment(const std::string& jsonString)
{
  if (jsonString.size() > 0) {
    simdjson::dom::parser p;
    auto element = p.parse(jsonString).value();
    pImpl->changeForm.equipment = Equipment::FromJson(element);
  } else
    pImpl->changeForm.equipment.reset();
}

void MpActor::SendToUser(const void* data, size_t size, bool reliable)
{
  if (sendToUser)
    sendToUser(this, data, size, reliable);
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
  static_cast<MpChangeFormACHR&>(res) = pImpl->changeForm;
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
  pImpl->changeForm = static_cast<const MpChangeFormACHR&>(changeForm);
}

const bool& MpActor::IsRaceMenuOpen() const
{
  return pImpl->changeForm.isRaceMenuOpen;
}

Look* MpActor::GetLook() const
{
  return pImpl->changeForm.look ? &*pImpl->changeForm.look : nullptr;
}

std::string MpActor::GetLookAsJson()
{
  if (GetLook())
    return GetLook()->ToJson();
  return "";
}

std::string MpActor::GetEquipmentAsJson()
{
  if (pImpl->changeForm.equipment)
    return pImpl->changeForm.equipment->ToJson().dump();
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