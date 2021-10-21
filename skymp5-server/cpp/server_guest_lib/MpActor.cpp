#include "MpActor.h"
#include "ChangeFormGuard.h"
#include "EspmGameObject.h"
#include "FormCallbacks.h"
#include "GetBaseActorValues.h"
#include "WorldState.h"
#include <MsgType.h>
#include <NiPoint3.h>

struct MpActor::Impl : public ChangeFormGuard<MpChangeForm>
{
  Impl(MpChangeForm changeForm_, MpObjectReference* self_)
    : ChangeFormGuard(changeForm_, self_)
  {
  }

  std::map<uint32_t, Viet::Promise<VarValue>> snippetPromises;
  uint32_t snippetIndex = 0;
  bool isRespawning = false;
  std::chrono::steady_clock::time_point lastAttributesUpdateTimePoint;
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

void MpActor::SetAppearance(const Appearance* newAppearance)
{
  pImpl->EditChangeForm([&](MpChangeForm& changeForm) {
    if (newAppearance)
      changeForm.appearanceDump = newAppearance->ToJson();
    else
      changeForm.appearanceDump.clear();
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
  uint32_t raceId = GetAppearance() ? GetAppearance()->raceId : 0;
  BaseActorValues baseActorValues;
  WorldState* worldState = GetParent();
  // this "if" is needed for unit testing: tests can call VisitProperties
  // without espm attached, which will cause tests to fail
  if (worldState && worldState->HasEspm()) {
    baseActorValues = GetBaseActorValues(worldState, baseId, raceId);
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
  res.appearanceDump = achr.appearanceDump;
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

      // Actor without appearance would not be visible so we force player to
      // choose appearance
      if (cf.appearanceDump.empty())
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
  if (healthPercentage == 0.f) {
    Kill();
    RespawnAfter(kRespawnTimeSeconds);
  }
}

std::chrono::steady_clock::time_point
MpActor::GetLastAttributesPercentagesUpdate()
{
  return pImpl->lastAttributesUpdateTimePoint;
}

void MpActor::SetLastAttributesPercentagesUpdate(
  std::chrono::steady_clock::time_point timePoint)
{
  pImpl->lastAttributesUpdateTimePoint = timePoint;
}

std::chrono::duration<float> MpActor::GetDurationOfAttributesPercentagesUpdate(
  std::chrono::steady_clock::time_point now)
{
  std::chrono::duration<float> timeAfterRegeneration =
    now - pImpl->lastAttributesUpdateTimePoint;
  return timeAfterRegeneration;
}

const bool& MpActor::IsRaceMenuOpen() const
{
  return pImpl->ChangeForm().isRaceMenuOpen;
}

const bool& MpActor::IsDead() const
{
  return pImpl->ChangeForm().isDead;
}

const bool& MpActor::IsRespawning() const
{
  return pImpl->isRespawning;
}

std::unique_ptr<const Appearance> MpActor::GetAppearance() const
{
  auto& changeForm = pImpl->ChangeForm();
  if (changeForm.appearanceDump.size() > 0) {
    simdjson::dom::parser p;
    auto doc = p.parse(changeForm.appearanceDump).value();

    std::unique_ptr<const Appearance> res;
    res.reset(new Appearance(Appearance::FromJson(doc)));
    return res;
  }
  return nullptr;
}

const std::string& MpActor::GetAppearanceAsJson()
{
  return pImpl->ChangeForm().appearanceDump;
}

const std::string& MpActor::GetEquipmentAsJson()
{
  return pImpl->ChangeForm().equipmentDump;
};

bool MpActor::IsWeaponDrawn() const
{
  return GetAnimationVariableBool("_skymp_isWeapDrawn");
}

void MpActor::SetAndSendIsDeadPropery(bool value)
{
  float health = value ? 0.f : 1.f;

  std::string isDeadMsg;
  isDeadMsg += Networking::MinPacketId;
  isDeadMsg += nlohmann::json{
    { "idx", GetIdx() },
    { "t", MsgType::UpdateProperty },
    { "propName", "isDead" },
    { "data", value }
  }.dump();

  std::string healthPercentageMsg;
  healthPercentageMsg += Networking::MinPacketId;
  healthPercentageMsg += nlohmann::json{
    { "idx", GetIdx() },
    { "t", MsgType::UpdateProperty },
    { "propName", "healthPercentage" },
    { "data", health }
  }.dump();

  SendToUser(isDeadMsg.data(), isDeadMsg.size(), true);
  SendToUser(healthPercentageMsg.data(), healthPercentageMsg.size(), true);

  pImpl->EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.isDead = value;
    changeForm.healthPercentage = health;
  });
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

void MpActor::Kill()
{
  SetAndSendIsDeadPropery(true);
}

void MpActor::RespawnAfter(float seconds)
{
  pImpl->isRespawning = true;

  uint32_t formId = GetFormId();
  if (auto worldState = GetParent()) {
    worldState->SetTimer(seconds).Then([worldState, this, formId](Viet::Void) {
      if (&worldState->GetFormAt<MpActor>(formId) == this) {
        this->Respawn();
      }
    });
  }
}

void MpActor::Respawn()
{
  pImpl->isRespawning = false;
  static const LocationalData position = { { 133857, -61130, 14662 },
                                           { 0.f, 0.f, 72.f },
                                           0x3C };
  TeleportUser(position);
  SetAndSendIsDeadPropery(false);
}

void MpActor::TeleportUser(LocationalData position)
{
  std::string teleportMsg;
  teleportMsg += Networking::MinPacketId;
  teleportMsg += nlohmann::json{
    { "pos", { position.pos[0], position.pos[1], position.pos[2] } },
    { "rot", { position.rot[0], position.rot[1], position.rot[2] } },
    { "worldOrCell", position.cellOrWorld },
    { "type", "teleport" }
  }.dump();
  SendToUser(teleportMsg.data(), teleportMsg.size(), true);

  SetCellOrWorldObsolete(position.cellOrWorld);
  SetPos(position.pos);
  SetAngle(position.rot);
}
