#include "MpActor.h"
#include "ActiveMagicEffectsMap.h"
#include "ActorValues.h"
#include "ChangeFormGuard.h"
#include "CropRegeneration.h"
#include "EspmGameObject.h"
#include "FormCallbacks.h"
#include "GetBaseActorValues.h"
#include "MathUtils.h"
#include "MpChangeForms.h"
#include "MsgType.h"
#include "PapyrusObjectReference.h"
#include "ServerState.h"
#include "SweetPieScript.h"
#include "TimeUtils.h"
#include "WorldState.h"
#include "libespm/espm.h"
#include <NiPoint3.h>
#include <chrono>
#include <functional>
#include <optional>
#include <random>
#include <string>

#include "ChangeValuesMessage.h"
#include "TeleportMessage.h"

struct MpActor::Impl
{
  std::map<uint32_t, Viet::Promise<VarValue>> snippetPromises;
  std::set<std::shared_ptr<DestroyEventSink>> destroyEventSinks;
  uint32_t snippetIndex = 0;
  bool isRespawning = false;
  bool isBlockActive = false;
  std::chrono::steady_clock::time_point lastAttributesUpdateTimePoint,
    lastHitTimePoint;
  using RestorationTimePoints =
    std::unordered_map<espm::ActorValue,
                       std::chrono::steady_clock::time_point>;
  RestorationTimePoints restorationTimePoints = {
    { espm::ActorValue::Health, std::chrono::steady_clock::time_point{} },
    { espm::ActorValue::Stamina, std::chrono::steady_clock::time_point{} },
    { espm::ActorValue::Magicka, std::chrono::steady_clock::time_point{} },
    { espm::ActorValue::HealRate, std::chrono::steady_clock::time_point{} },
    { espm::ActorValue::MagickaRate, std::chrono::steady_clock::time_point{} },
    { espm::ActorValue::StaminaRate, std::chrono::steady_clock::time_point{} },
    { espm::ActorValue::HealRateMult_or_CombatHealthRegenMultMod,
      std::chrono::steady_clock::time_point{} },
    { espm::ActorValue::MagickaRateMult_or_CombatHealthRegenMultPowerMod,
      std::chrono::steady_clock::time_point{} },
    { espm::ActorValue::StaminaRateMult,
      std::chrono::steady_clock::time_point{} },
  };
  uint32_t blockActiveCount = 0;
};

MpActor::MpActor(const LocationalData& locationalData_,
                 const FormCallbacks& callbacks_, uint32_t optBaseId)
  : MpObjectReference(locationalData_, callbacks_,
                      optBaseId == 0 ? 0x7 : optBaseId, "NPC_")
{
  pImpl.reset(new Impl);
}

void MpActor::IncreaseBlockCount() noexcept
{
  ++pImpl->blockActiveCount;
}

void MpActor::ResetBlockCount() noexcept
{
  pImpl->blockActiveCount = 0;
}

uint32_t MpActor::GetBlockCount() const noexcept
{
  return pImpl->blockActiveCount;
}

bool MpActor::GetConsoleCommandsAllowedFlag() const
{
  return GetChangeForm().consoleCommandsAllowed;
}

void MpActor::SetConsoleCommandsAllowedFlag(bool newValue)
{
  EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.consoleCommandsAllowed = newValue;
  });
}

void MpActor::SetRaceMenuOpen(bool isOpen)
{
  EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.isRaceMenuOpen = isOpen; });
}

void MpActor::SetAppearance(const Appearance* newAppearance)
{
  EditChangeForm([&](MpChangeForm& changeForm) {
    if (newAppearance)
      changeForm.appearanceDump = newAppearance->ToJson();
    else
      changeForm.appearanceDump.clear();
  });
}

void MpActor::SetEquipment(const std::string& jsonString)
{
  EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.equipmentDump = jsonString; });
}

void MpActor::VisitProperties(const PropertiesVisitor& visitor,
                              VisitPropertiesMode mode)
{
  const auto baseId = GetBaseId();
  const uint32_t raceId = GetAppearance() ? GetAppearance()->raceId : 0;

  BaseActorValues baseActorValues;
  WorldState* worldState = GetParent();
  // this "if" is needed for unit testing: tests can call VisitProperties
  // without espm attached, which will cause tests to fail
  if (worldState && worldState->HasEspm()) {
    baseActorValues = GetBaseActorValues(worldState, baseId, raceId);
  }

  MpChangeForm changeForm = GetChangeForm();

  MpObjectReference::VisitProperties(visitor, mode);

  if (mode == VisitPropertiesMode::All && IsRaceMenuOpen()) {
    visitor("isRaceMenuOpen", "true");
  }

  if (mode == VisitPropertiesMode::All) {
    baseActorValues.VisitBaseActorValues(baseActorValues, changeForm, visitor);
  }

  visitor("learnedSpells",
          nlohmann::json(changeForm.learnedSpells.GetLearnedSpells())
            .dump()
            .c_str());
}

void MpActor::Disable()
{
  if (ChangeForm().isDisabled) {
    return;
  }

  MpObjectReference::Disable();

  for (auto [snippetIdx, promise] : pImpl->snippetPromises) {
    spdlog::warn("Disabling actor {:x} with pending snippet promise",
                 GetFormId());
    try {
      promise.Resolve(VarValue::None());
    } catch (std::exception& e) {
      // Not sure if this is possible, but better safe than sorry
      spdlog::error("Exception while resolving pending snippet promise: {}",
                    e.what());
    }
  }

  pImpl->snippetPromises.clear();
}

void MpActor::SendToUser(const IMessageBase& message, bool reliable)
{
  if (callbacks->sendToUser) {
    callbacks->sendToUser(this, message, reliable);
  } else {
    throw std::runtime_error("sendToUser is nullptr");
  }
}

void MpActor::SendToUserDeferred(const void* data, size_t size, bool reliable,
                                 int deferredChannelId,
                                 bool overwritePreviousChannelMessages)
{
  if (callbacks->sendToUserDeferred) {
    callbacks->sendToUserDeferred(this, data, size, reliable,
                                  deferredChannelId,
                                  overwritePreviousChannelMessages);
  } else {
    throw std::runtime_error("sendToUserDeferred is nullptr");
  }
}

bool MpActor::OnEquip(uint32_t baseId)
{
  const auto& espm = GetParent()->GetEspm();
  auto lookupRes = espm.GetBrowser().LookupById(baseId);

  if (!lookupRes.rec) {
    return false;
  }

  const auto recordType = lookupRes.rec->GetType();

  const bool isSpell = recordType == "SPEL";
  const bool isBook = recordType == "BOOK";
  const bool isIngredient = recordType == "INGR";
  const bool isPotion = recordType == "ALCH";

  if (!(isSpell || isIngredient || isPotion || isBook)) {
    return false;
  }

  const bool hasItem = isSpell
    ? GetChangeForm().learnedSpells.IsSpellLearned(baseId)
    : GetInventory().GetItemCount(baseId) > 0;

  if (!hasItem) {
    return false;
  }

  bool spellLearned = true;
  if (isIngredient || isPotion) {
    EatItem(baseId, recordType);
  } else if (isBook) {
    spellLearned = ReadBook(baseId);
  }

  if (!isSpell && spellLearned) {
    RemoveItem(baseId, 1, nullptr);
  }

  const VarValue args[] = {
    VarValue(std::make_shared<EspmGameObject>(lookupRes)), VarValue::None()
  };

  SendPapyrusEvent("OnObjectEquipped", args, std::size(args));

  const auto& espmFiles = GetParent()->espmFiles;

  if (std::any_of(espmFiles.begin(), espmFiles.end(),
                  [](auto&& element) { return element == "SweetPie.esp"; })) {
    SweetPieScript SweetPieScript(espmFiles);
    SweetPieScript.Play(*this, *GetParent(), baseId);
  }

  return true;
}

void MpActor::AddEventSink(std::shared_ptr<DestroyEventSink> sink)
{
  pImpl->destroyEventSinks.insert(sink);
}

void MpActor::RemoveEventSink(std::shared_ptr<DestroyEventSink> sink)
{
  pImpl->destroyEventSinks.erase(sink);
}

MpChangeForm MpActor::GetChangeForm() const
{
  auto res = MpObjectReference::GetChangeForm();
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
  EditChangeForm(
    [&](MpChangeForm& changeForm) {
      changeForm = static_cast<const MpChangeForm&>(newChangeForm);

      // Actor without appearance would not be visible so we force player to
      // choose appearance
      if (changeForm.appearanceDump.empty()) {
        changeForm.isRaceMenuOpen = true;
      }
      // ActorValues does not refelect real base actor values set in esp/esm
      // game files since new update
      // this check is added only for test as a workaround. It is to be redone
      // in the nearest future. TODO
      if (GetParent() && GetParent()->HasEspm()) {
        changeForm.actorValues =
          GetBaseActorValues(GetParent(), GetBaseId(), GetRaceId());
      }
    },
    Mode::NoRequestSave);
  ReapplyMagicEffects();
}

uint32_t MpActor::NextSnippetIndex(
  std::optional<Viet::Promise<VarValue>> promise)
{
  auto res = pImpl->snippetIndex++;
  if (promise) {
    pImpl->snippetPromises[res] = *promise;
  }
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

void MpActor::SetPercentages(const ActorValues& actorValues,
                             MpActor* aggressor)
{
  if (IsDead() || pImpl->isRespawning) {
    return;
  }
  if (actorValues.healthPercentage == 0.f) {
    Kill(aggressor);
    return;
  }
  EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.actorValues.healthPercentage = actorValues.healthPercentage;
    changeForm.actorValues.magickaPercentage = actorValues.magickaPercentage;
    changeForm.actorValues.staminaPercentage = actorValues.staminaPercentage;
  });
  SetLastAttributesPercentagesUpdate(std::chrono::steady_clock::now());
}

void MpActor::NetSendChangeValues(const ActorValues& actorValues)
{
  ChangeValuesMessage message;
  message.idx = GetIdx();
  message.health = actorValues.healthPercentage;
  message.magicka = actorValues.magickaPercentage;
  message.stamina = actorValues.staminaPercentage;
  SendToUser(message, true);
}

void MpActor::NetSetPercentages(const ActorValues& actorValues,
                                MpActor* aggressor)
{
  NetSendChangeValues(actorValues);
  SetPercentages(actorValues, aggressor);
}

std::chrono::steady_clock::time_point
MpActor::GetLastAttributesPercentagesUpdate()
{
  return pImpl->lastAttributesUpdateTimePoint;
}

std::chrono::steady_clock::time_point MpActor::GetLastHitTime()
{
  return pImpl->lastHitTimePoint;
}

void MpActor::SetLastAttributesPercentagesUpdate(
  std::chrono::steady_clock::time_point timePoint)
{
  pImpl->lastAttributesUpdateTimePoint = timePoint;
}

void MpActor::SetLastHitTime(std::chrono::steady_clock::time_point timePoint)
{
  pImpl->lastHitTimePoint = timePoint;
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
  return ChangeForm().isRaceMenuOpen;
}

const bool& MpActor::IsDead() const
{
  return ChangeForm().isDead;
}

const bool& MpActor::IsRespawning() const
{
  return pImpl->isRespawning;
}

bool MpActor::IsSpellLearned(const uint32_t baseId) const
{
  const auto npcData = espm::GetData<espm::NPC_>(GetBaseId(), GetParent());
  const auto raceData = espm::GetData<espm::RACE>(npcData.race, GetParent());

  return npcData.spells.count(baseId) || raceData.spells.count(baseId) ||
    ChangeForm().learnedSpells.IsSpellLearned(baseId);
}

std::unique_ptr<const Appearance> MpActor::GetAppearance() const
{
  auto& changeForm = ChangeForm();
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
  return ChangeForm().appearanceDump;
}

const std::string& MpActor::GetEquipmentAsJson() const
{
  return ChangeForm().equipmentDump;
}

Equipment MpActor::GetEquipment() const
{
  simdjson::dom::parser p;

  return Equipment::FromJson(p.parse(GetEquipmentAsJson()).value());
}

uint32_t MpActor::GetRaceId() const
{
  const auto appearance = GetAppearance();

  if (appearance) {
    return appearance->raceId;
  }

  return espm::GetData<espm::NPC_>(GetBaseId(), GetParent()).race;
}

bool MpActor::IsWeaponDrawn() const
{
  return GetAnimationVariableBool("_skymp_isWeapDrawn");
}

espm::ObjectBounds MpActor::GetBounds() const
{
  return espm::GetData<espm::NPC_>(GetBaseId(), GetParent()).objectBounds;
}

void MpActor::SendAndSetDeathState(bool isDead, bool shouldTeleport)
{
  float attribute = isDead ? 0.f : 1.f;
  auto position = GetSpawnPoint();

  auto respawnMsg = GetDeathStateMsg(position, isDead, shouldTeleport);
  SendToUser(respawnMsg, true);

  EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.isDead = isDead;
    changeForm.actorValues.healthPercentage = attribute;
    changeForm.actorValues.magickaPercentage = attribute;
    changeForm.actorValues.staminaPercentage = attribute;
  });
  if (shouldTeleport) {
    SetCellOrWorldObsolete(position.cellOrWorldDesc);
    SetPos(position.pos);
    SetAngle(position.rot);
  }
}

DeathStateContainerMessage MpActor::GetDeathStateMsg(
  const LocationalData& position, bool isDead, bool shouldTeleport)
{
  DeathStateContainerMessage res;
  res.tIsDead = PreparePropertyMessage(this, "isDead", isDead);

  if (shouldTeleport) {
    res.tTeleport = TeleportMessage();
    res.tTeleport->idx = GetIdx();
    std::copy(&position.pos[0], &position.pos[0] + 3,
              std::begin(res.tTeleport->pos));
    std::copy(&position.rot[0], &position.rot[0] + 3,
              std::begin(res.tTeleport->rot));
    res.tTeleport->worldOrCell =
      position.cellOrWorldDesc.ToFormId(GetParent()->espmFiles);
  }

  if (!isDead) {
    constexpr float kAttributePercentageFull = 1.f;
    res.tChangeValues = ChangeValuesMessage();
    res.tChangeValues->idx = GetIdx();
    res.tChangeValues->health = kAttributePercentageFull;
    res.tChangeValues->magicka = kAttributePercentageFull;
    res.tChangeValues->stamina = kAttributePercentageFull;
  }

  return res;
}

void MpActor::MpApiDeath(MpActor* killer)
{
  simdjson::dom::parser parser;
  bool isRespawnBlocked = false;

  std::string s =
    "[" + std::to_string(killer ? killer->GetFormId() : 0) + " ]";
  auto args = parser.parse(s).value();

  if (auto wst = GetParent()) {
    const auto id = GetFormId();
    for (auto& listener : wst->listeners) {
      if (listener->OnMpApiEvent("onDeath", args, id) == false) {
        isRespawnBlocked = true;
      };
    }
  }
  if (!isRespawnBlocked) {
    RespawnWithDelay();
  }
}

void MpActor::EatItem(uint32_t baseId, espm::Type t)
{
  auto espmProvider = GetParent();
  std::vector<espm::Effects::Effect> effects;
  if (t == "ALCH") {
    effects = espm::GetData<espm::ALCH>(baseId, espmProvider).effects;
  } else if (t == "INGR") {
    effects = espm::GetData<espm::INGR>(baseId, espmProvider).effects;
  } else {
    return;
  }
  std::unordered_set<std::string> modFiles = { GetParent()->espmFiles.begin(),
                                               GetParent()->espmFiles.end() };
  bool hasSweetpie = modFiles.count("SweetPie.esp");
  ApplyMagicEffects(effects, hasSweetpie);
}

bool MpActor::ReadBook(const uint32_t baseId)
{
  const auto bookData = espm::GetData<espm::BOOK>(baseId, GetParent());

  if (bookData.IsFlagSet(espm::BOOK::Flags::TeachesSpell)) {

    EditChangeForm([&](MpChangeForm& changeForm) {
      changeForm.learnedSpells.LearnSpell(bookData.spellOrSkillFormId);
    });
    return true;
  }
  return false;
}

bool MpActor::CanActorValueBeRestored(espm::ActorValue av)
{
  if (std::chrono::steady_clock::now() - GetLastRestorationTime(av) <
      std::chrono::minutes(1)) {
    return false;
  }
  SetLastRestorationTime(av, std::chrono::steady_clock::now());
  return true;
}

std::chrono::steady_clock::time_point MpActor::GetLastRestorationTime(
  espm::ActorValue av) const noexcept
{
  return pImpl->restorationTimePoints[av];
}

void MpActor::SetLastRestorationTime(
  espm::ActorValue av, std::chrono::steady_clock::time_point timePoint)
{
  pImpl->restorationTimePoints[av] = timePoint;
}

void MpActor::ModifyActorValuePercentage(espm::ActorValue av,
                                         float percentageDelta)
{
  ActorValues currentActorValues = GetChangeForm().actorValues;
  switch (av) {
    case espm::ActorValue::Health:
      currentActorValues.healthPercentage =
        CropValue(currentActorValues.healthPercentage + percentageDelta);
      break;
    case espm::ActorValue::Stamina:
      currentActorValues.staminaPercentage =
        CropValue(currentActorValues.staminaPercentage + percentageDelta);
      break;
    case espm::ActorValue::Magicka:
      currentActorValues.magickaPercentage =
        CropValue(currentActorValues.magickaPercentage + percentageDelta);
      break;
    default:
      return;
  }
  NetSetPercentages(currentActorValues);
}

void MpActor::BeforeDestroy()
{
  for (auto& sink : pImpl->destroyEventSinks)
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

void MpActor::Kill(MpActor* killer, bool shouldTeleport)
{
  SendAndSetDeathState(true, shouldTeleport);
  MpApiDeath(killer);
}

void MpActor::RespawnWithDelay(bool shouldTeleport)
{
  if (pImpl->isRespawning) {
    return;
  }
  pImpl->isRespawning = true;

  uint32_t formId = GetFormId();
  if (auto worldState = GetParent()) {
    float respawnTime = GetRespawnTime();
    auto time = Viet::TimeUtils::To<std::chrono::milliseconds>(respawnTime);
    worldState->SetTimer(time).Then(
      [worldState, this, formId, shouldTeleport](Viet::Void) {
        if (worldState->LookupFormById(formId).get() == this) {
          this->Respawn(shouldTeleport);
        }
      });
  }
}

void MpActor::Respawn(bool shouldTeleport)
{
  if (IsDead() == false) {
    return;
  }
  pImpl->isRespawning = false;
  SendAndSetDeathState(false, shouldTeleport);
}

void MpActor::Teleport(const LocationalData& position)
{
  TeleportMessage msg;
  msg.idx = GetIdx();
  std::copy(&position.pos[0], &position.pos[0] + 3, std::begin(msg.pos));
  std::copy(&position.rot[0], &position.rot[0] + 3, std::begin(msg.rot));
  msg.worldOrCell = position.cellOrWorldDesc.ToFormId(GetParent()->espmFiles);
  SendToUser(msg, true);

  SetCellOrWorldObsolete(position.cellOrWorldDesc);
  SetPos(position.pos);
  SetAngle(position.rot);
}

void MpActor::SetSpawnPoint(const LocationalData& position)
{
  EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.spawnPoint = position; });
}

LocationalData MpActor::GetSpawnPoint() const
{
  return ChangeForm().spawnPoint;
}

const float MpActor::GetRespawnTime() const
{
  return ChangeForm().spawnDelay;
}

void MpActor::SetRespawnTime(float time)
{
  EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.spawnDelay = time; });
}

void MpActor::SetIsDead(bool isDead)
{
  constexpr bool kShouldTeleport = false;

  if (isDead) {
    if (IsDead() == false) {
      SendAndSetDeathState(isDead, kShouldTeleport);
    }
  } else {
    // same as SendAndSetDeathState but resets isRespawning flag
    Respawn(kShouldTeleport);
  }
}

void MpActor::RestoreActorValue(espm::ActorValue av, float value)
{
  ModifyActorValuePercentage(
    av, std::abs(value) / GetMaximumValues().GetValue(av));
}

void MpActor::DamageActorValue(espm::ActorValue av, float value)
{
  ModifyActorValuePercentage(
    av, -std::abs(value) / GetMaximumValues().GetValue(av));
}

BaseActorValues MpActor::GetBaseValues()
{
  return GetBaseActorValues(GetParent(), GetBaseId(), GetRaceId());
}

BaseActorValues MpActor::GetMaximumValues()
{
  return GetBaseValues();
}

void MpActor::DropItem(const uint32_t baseId, const Inventory::Entry& entry)
{
  // TODO: Take count into account
  int count = entry.count;
  RemoveItems({ entry });
  // TODO(#1141): reimplement spawning items
}

void MpActor::SetIsBlockActive(bool active)
{
  pImpl->isBlockActive = active;
}

bool MpActor::IsBlockActive() const
{
  return pImpl->isBlockActive;
}

const float kAngleToRadians = std::acos(-1.f) / 180.f;

NiPoint3 MpActor::GetViewDirection() const
{
  return { std::sin(GetAngle().z * kAngleToRadians),
           std::cos(GetAngle().z * kAngleToRadians), 0 };
}

void MpActor::SetActorValue(espm::ActorValue actorValue, float value)
{
  ActorValues currentActorValues = GetChangeForm().actorValues;
  switch (actorValue) {
    case espm::ActorValue::HealRate:
      currentActorValues.healRate = value;
      break;
    case espm::ActorValue::MagickaRate:
      currentActorValues.magickaRate = value;
      break;
    case espm::ActorValue::StaminaRate:
      currentActorValues.staminaRate = value;
      break;
    case espm::ActorValue::Health:
      currentActorValues.health = value;
      break;
    case espm::ActorValue::Magicka:
      currentActorValues.magicka = value;
      break;
    case espm::ActorValue::Stamina:
      currentActorValues.stamina = value;
      break;
    case espm::ActorValue::HealRateMult_or_CombatHealthRegenMultMod:
      currentActorValues.healRateMult = value;
      break;
    case espm::ActorValue::StaminaRateMult:
      currentActorValues.staminaRateMult = value;
      break;
    case espm::ActorValue::MagickaRateMult_or_CombatHealthRegenMultPowerMod:
      currentActorValues.magickaRateMult = value;
      break;
    default:
      break;
  }
  NetSendChangeValues(currentActorValues);
  EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.actorValues = currentActorValues;
  });
}

void MpActor::SetActorValues(const ActorValues& actorValues)
{
  NetSendChangeValues(actorValues);
  EditChangeForm(
    [&](MpChangeForm& changeForm) { changeForm.actorValues = actorValues; });
}

void MpActor::ApplyMagicEffect(espm::Effects::Effect& effect, bool hasSweetpie,
                               bool durationOverriden)
{
  WorldState* worldState = GetParent();
  auto data = espm::GetData<espm::MGEF>(effect.effectId, worldState).data;
  const espm::ActorValue av = data.primaryAV;
  const espm::MGEF::EffectType type = data.effectType;
  spdlog::trace("Actor value in ApplyMagicEffect(): {}",
                static_cast<std::underlying_type_t<espm::ActorValue>>(av));

  const bool isValue = av == espm::ActorValue::Health ||
    av == espm::ActorValue::Stamina || av == espm::ActorValue::Magicka;
  const bool isRate = av == espm::ActorValue::HealRate ||
    av == espm::ActorValue::StaminaRate || av == espm::ActorValue::MagickaRate;
  const bool isMult =
    av == espm::ActorValue::HealRateMult_or_CombatHealthRegenMultMod ||
    av == espm::ActorValue::StaminaRateMult ||
    av == espm::ActorValue::MagickaRateMult_or_CombatHealthRegenMultPowerMod;

  if (isValue) { // other types are unsupported
    if (hasSweetpie) {
      if (CanActorValueBeRestored(av)) {
        // this coefficient (workaround) has been added for sake of game
        // balance and because of disability to restrict players use potions
        // often on client side
        constexpr float kMagnitudeCoeff = 100.f;
        RestoreActorValue(av, effect.magnitude * kMagnitudeCoeff);
      }
    } else {
      RestoreActorValue(av, effect.magnitude);
    }
  }

  if (isRate || isMult) {
    if (hasSweetpie && !CanActorValueBeRestored(av)) {
      return;
    }
    MpChangeForm changeForm = GetChangeForm();
    BaseActorValues baseValues =
      GetBaseActorValues(GetParent(), GetBaseId(), GetRaceId());
    const ActiveMagicEffectsMap& activeEffects = changeForm.activeMagicEffects;
    const float baseValue = baseValues.GetValue(av);
    const uint32_t formId = GetFormId();
    auto now = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point endTime;
    std::chrono::milliseconds duration;
    if (durationOverriden) {
      std::optional effect = GetChangeForm().activeMagicEffects.Get(av);
      if (!effect.has_value()) {
        spdlog::error(
          "MpActor with formId {:x} has no magic effect affecting "
          "actor value {}",
          GetFormId(),
          static_cast<std::underlying_type_t<espm::ActorValue>>(av));
        return;
      }
      endTime = effect.value().get().endTime;
      duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - now);
    } else {
      endTime =
        now + Viet::TimeUtils::To<std::chrono::milliseconds>(effect.duration);
      duration =
        Viet::TimeUtils::To<std::chrono::milliseconds>(effect.duration);
    }
    uint32_t timerId;
    worldState->SetEffectTimer(duration, &timerId)
      .Then([formId, actorValue = av, worldState](Viet::Void) {
        auto& actor = worldState->GetFormAt<MpActor>(formId);
        actor.RemoveMagicEffect(actorValue);
      });

    ActiveMagicEffectsMap::Entry entry{ timerId, effect, endTime };
    if (activeEffects.Has(av)) {
      const ActiveMagicEffectsMap::Entry& entry =
        activeEffects.Get(av).value().get();
      worldState->RemoveEffectTimer(entry.timerId);
    }
    EditChangeForm([av, pEntry = &entry](MpChangeForm& changeForm) {
      changeForm.activeMagicEffects.Add(av, *pEntry);
    });
    if (isRate) {
      SetActorValue(av, effect.magnitude);
    } else {
      float mult = 1.f;
      if (type == espm::MGEF::EffectType::PeakValueMod) {
        mult = MathUtils::PercentToMultPos(effect.magnitude);
      }

      if (type == espm::MGEF::EffectType::ValueMod) {
        mult = MathUtils::PercentToMultNeg(effect.magnitude);
      }
      if (MathUtils::IsNearlyEqual(1.f, mult)) {
        spdlog::error(
          "Unknown espm::MGEF::EffectType: {}",
          static_cast<std::underlying_type_t<espm::MGEF::EffectType>>(type));
      }
      spdlog::trace("Final multiplicator is {}", mult);
      spdlog::trace("The result of baseValue * mult is: {}*{}={}", baseValue,
                    mult, baseValue * mult);
      SetActorValue(av, baseValue * mult);
    }
  }
}

void MpActor::ApplyMagicEffects(std::vector<espm::Effects::Effect>& effects,
                                bool hasSweetpie, bool durationOverriden)
{
  for (auto& effect : effects) {
    ApplyMagicEffect(effect, hasSweetpie, durationOverriden);
  }
}

void MpActor::RemoveMagicEffect(const espm::ActorValue actorValue) noexcept
{
  const ActorValues baseActorValues =
    GetBaseActorValues(GetParent(), GetBaseId(), GetRaceId());
  const float baseActorValue = baseActorValues.GetValue(actorValue);
  SetActorValue(actorValue, baseActorValue);
  EditChangeForm([actorValue](MpChangeForm& changeForm) {
    changeForm.activeMagicEffects.Remove(actorValue);
  });
}

void MpActor::RemoveAllMagicEffects() noexcept
{
  const ActorValues baseActorValues =
    GetBaseActorValues(GetParent(), GetBaseId(), GetRaceId());
  SetActorValues(baseActorValues);
  EditChangeForm(
    [](MpChangeForm& changeForm) { changeForm.activeMagicEffects.Clear(); });
}

void MpActor::ReapplyMagicEffects()
{
  // TODO: Implement range-based for loop for MagicEffectsMap
  std::vector<espm::Effects::Effect> activeEffects =
    GetChangeForm().activeMagicEffects.GetAllEffects();
  if (activeEffects.empty()) {
    return;
  }
  const std::vector<std::string>& modFiles = GetParent()->espmFiles;
  const bool hasSweetpie = std::any_of(
    modFiles.begin(), modFiles.end(),
    [](std::string_view fileName) { return fileName == "SweetPie.esp"; });
  ApplyMagicEffects(activeEffects, hasSweetpie, true);
}

std::array<std::optional<Inventory::Entry>, 2> MpActor::GetEquippedWeapon()
  const
{
  std::array<std::optional<Inventory::Entry>, 2> wornWeaponEntries;
  // 0 -> left hand, 1 -> right hand
  auto& espmBrowser = GetParent()->GetEspm().GetBrowser();
  for (const auto& entry : GetEquipment().inv.entries) {
    if (entry.extra.worn != Inventory::Worn::None) {
      espm::LookupResult res = espmBrowser.LookupById(entry.baseId);
      auto* weaponRecord = espm::Convert<espm::WEAP>(res.rec);
      if (weaponRecord) {
        if (entry.extra.worn == Inventory::Worn::Left) {
          wornWeaponEntries[0] = std::move(entry);
        }
        if (entry.extra.worn == Inventory::Worn::Right) {
          wornWeaponEntries[1] = std::move(entry);
        }
      }
    }
  }
  return wornWeaponEntries;
}
