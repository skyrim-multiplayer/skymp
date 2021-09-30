#include "MpActor.h"
#include "ChangeFormGuard.h"
#include "EspmGameObject.h"
#include "FormCallbacks.h"
#include "GetBaseActorValues.h"
#include "WorldState.h"
#include <NiPoint3.h>

struct MpActor::Impl : public ChangeFormGuard<MpChangeForm>
{
  Impl(MpChangeForm changeForm_, MpObjectReference* self_)
    : ChangeFormGuard(changeForm_, self_)
  {
  }

  std::map<uint32_t, Viet::Promise<VarValue>> snippetPromises;
  uint32_t snippetIndex = 0;
};

MpActor::MpActor(const LocationalData& locationalData_,
                 const FormCallbacks& callbacks_, uint32_t optBaseId)
  : MpObjectReference(locationalData_, callbacks_,
                      optBaseId == 0 ? 0x7 : optBaseId, "NPC_")
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
  auto baseId = MpObjectReference::GetBaseId();
  uint32_t raceId = GetLook() ? GetLook()->raceId : 0;
  BaseActorValues baseActorValues;
  WorldState* worldState = GetParent();
  if (worldState && worldState->HasEspm()) {
    auto& espm = worldState->GetEspm();
    baseActorValues = GetBaseActorValues(espm, baseId, raceId);
  }

  MpChangeForm changeForm = GetChangeForm();

  MpObjectReference::VisitProperties(visitor, mode);
  if (mode == VisitPropertiesMode::All && IsRaceMenuOpen())
    visitor("isRaceMenuOpen", "true");

  if (mode == VisitPropertiesMode::All) {
    baseActorValues.VisitBaseActorValues(baseActorValues, changeForm, visitor);
  }
}

void MpActor::SendToUser(const void* data, size_t size, bool reliable)
{
  if (callbacks->sendToUser)
    callbacks->sendToUser(this, data, size, reliable);
  else
    throw std::runtime_error("sendToUser is nullptr");
}

void MpActor::OnEquip(uint32_t baseId)
{
  if (GetInventory().GetItemCount(baseId) == 0)
    return;
  auto& espm = GetParent()->GetEspm();
  auto lookupRes = espm.GetBrowser().LookupById(baseId);
  if (!lookupRes.rec)
    return;
  auto t = lookupRes.rec->GetType();
  if (t == "INGR" || t == "ALCH") {
    // Eat item
    RemoveItem(baseId, 1, nullptr);

    VarValue args[] = { VarValue(std::make_shared<EspmGameObject>(lookupRes)),
                        VarValue::None() };
    SendPapyrusEvent("OnObjectEquipped", args, std::size(args));
  }
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
  res.healthPercentage = achr.healthPercentage;
  res.magickaPercentage = achr.magickaPercentage;
  res.staminaPercentage = achr.staminaPercentage;
  // achr.dynamicFields isn't really used so I decided to comment this line:
  // res.dynamicFields.merge_patch(achr.dynamicFields);

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

      // Actor without look would not be visible so we force player to choose
      // appearance
      if (cf.lookDump.empty())
        cf.isRaceMenuOpen = true;
    },
    Impl::Mode::NoRequestSave);
}

uint32_t MpActor::NextSnippetIndex(
  std::optional<Viet::Promise<VarValue>> promise)
{
  auto res = pImpl->snippetIndex++;
  if (promise)
    pImpl->snippetPromises[res] = *promise;
  return res;
}

void MpActor::ResolveSnippet(uint32_t snippetIdx, VarValue v)
{
  auto it = pImpl->snippetPromises.find(snippetIdx);
  if (it != pImpl->snippetPromises.end()) {
    auto& promise = it->second;
    promise.Resolve(v);
    pImpl->snippetPromises.erase(it);
  }
}

void MpActor::SetPercentages(float healthPercentage, float magickaPercentage,
                             float staminaPercentage)
{
  pImpl->EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.healthPercentage = healthPercentage;
    changeForm.magickaPercentage = magickaPercentage;
    changeForm.staminaPercentage = staminaPercentage;
  });
}

std::chrono::steady_clock::time_point
MpActor::GetLastAttributesPercentagesUpdate()
{
  return lastAttributesUpdateTimePoint;
}

void MpActor::SetLastAttributesPercentagesUpdate(
  std::chrono::steady_clock::time_point timePoint)
{
  lastAttributesUpdateTimePoint = timePoint;
}

std::chrono::duration<float> MpActor::GetDurationOfAttributesPercentagesUpdate(
  std::chrono::steady_clock::time_point now)
{
  std::chrono::duration<float> timeAfterRegeneration =
    now - lastAttributesUpdateTimePoint;
  return timeAfterRegeneration;
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

bool MpActor::IsWeaponDrawn() const
{
  return GetAnimationVariableBool("_skymp_isWeapDrawn");
}

void MpActor::BeforeDestroy()
{
  for (auto& sink : destroyEventSinks)
    sink->BeforeDestroy(*this);

  MpObjectReference::BeforeDestroy();

  UnsubscribeFromAll();
}

void MpActor::Init(WorldState* worldState, uint32_t formId, bool hasChangeForm)
{
  MpObjectReference::Init(worldState, formId, hasChangeForm);

  if (worldState->HasEspm()) {
    EnsureBaseContainerAdded(GetParent()->GetEspm());
  }
}
