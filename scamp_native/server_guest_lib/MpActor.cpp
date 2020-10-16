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
  auto res = pImpl->ChangeForm();
  static_cast<MpChangeFormREFR&>(res) =
    static_cast<MpChangeFormREFR&>(MpObjectReference::GetChangeForm());

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
    [&](MpChangeForm& changeForm) {
      changeForm = static_cast<const MpChangeForm&>(changeForm);
    },
    Impl::Mode::NoRequestSave);
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