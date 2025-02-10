#include "MpActor.h"
#include "ActiveMagicEffectsMap.h"
#include "ActorValues.h"
#include "ChangeFormGuard.h"
#include "CropRegeneration.h"
#include "EvaluateTemplate.h"
#include "FormCallbacks.h"
#include "GetBaseActorValues.h"
#include "LeveledListUtils.h"
#include "LocationalDataUtils.h"
#include "MathUtils.h"
#include "MpChangeForms.h"
#include "MsgType.h"
#include "ServerState.h"
#include "SpSnippet.h"
#include "SpSnippetFunctionGen.h"
#include "SweetPieScript.h"
#include "TimeUtils.h"
#include "WorldState.h"
#include "gamemode_events/DeathEvent.h"
#include "gamemode_events/DropItemEvent.h"
#include "gamemode_events/EatItemEvent.h"
#include "gamemode_events/ReadBookEvent.h"
#include "gamemode_events/RespawnEvent.h"
#include "libespm/espm.h"
#include "papyrus-vm/Utils.h"
#include "script_objects/EspmGameObject.h"
#include <NiPoint3.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <random>
#include <string>

#include "ChangeValuesMessage.h"
#include "TeleportMessage.h"
#include "UpdateEquipmentMessage.h"

// for PlaceAtMe used in MpActor::DropItem
#include "script_classes/PapyrusObjectReference.h"
#include "script_objects/MpFormGameObject.h"

struct MpActor::Impl
{
  std::map<uint32_t, Viet::Promise<VarValue>> snippetPromises;
  std::set<std::shared_ptr<DestroyEventSink>> destroyEventSinks;
  uint32_t snippetIndex = 0;
  uint32_t respawnTimerIndex = 0;
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
  std::vector<std::pair<uint32_t, MpObjectReference*>> droppedItemsQueue;
  std::optional<AnimationData> animationData;

  // this is a hot fix attempt to make permanent restoration potions work
  std::chrono::system_clock::time_point nextRestorationTime{};
};

namespace {

void RestoreActorValuePatched(MpActor* actor, espm::ActorValue actorValue,
                              float value)
{
  actor->RestoreActorValue(actorValue, value);
  actor->UpdateNextRestorationTime(std::chrono::seconds{ 5 });
}

}

void MpActor::UpdateNextRestorationTime(std::chrono::seconds duration) noexcept
{
  pImpl->nextRestorationTime = std::chrono::system_clock::now() + duration;
}

bool MpActor::ShouldSkipRestoration() const noexcept
{
  return pImpl->nextRestorationTime > std::chrono::system_clock::now();
}

MpActor::MpActor(const LocationalData& locationalData_,
                 const FormCallbacks& callbacks_, uint32_t optBaseId)
  : MpObjectReference(locationalData_, callbacks_,
                      optBaseId == 0 ? 0x7 : optBaseId, "NPC_")
{
  pImpl.reset(new Impl);
  asActor = this;
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

void MpActor::EquipBestWeapon()
{
  if (!GetParent()->HasEspm()) {
    return;
  }

  auto& loader = GetParent()->GetEspm();
  auto& cache = GetParent()->GetEspmCache();

  const Equipment eq = GetEquipment();

  Equipment newEq;
  newEq.numChanges = eq.numChanges + 1;
  for (auto& entry : eq.inv.entries) {
    bool isEquipped = entry.GetWorn() != Inventory::Worn::None;
    bool isWeap =
      espm::GetRecordType(entry.baseId, GetParent()) == espm::WEAP::kType;
    if (isEquipped && isWeap) {
      continue;
    }
    newEq.inv.AddItems({ entry });
  }

  const Inventory& inv = GetInventory();
  Inventory::Entry bestEntry;
  int16_t bestDamage = -1;
  for (auto& entry : inv.entries) {
    if (entry.baseId) {
      auto lookupRes = loader.GetBrowser().LookupById(entry.baseId);
      if (auto weap = espm::Convert<espm::WEAP>(lookupRes.rec)) {
        if (!bestEntry.count ||
            weap->GetData(cache).weapData->damage > bestDamage) {
          bestEntry = entry;
          bestDamage = weap->GetData(cache).weapData->damage;
        }
      }
    }
  }

  if (bestEntry.count > 0) {
    bestEntry.SetWorn(Inventory::Worn::Right);
    newEq.inv.AddItems({ bestEntry });
  }

  SetEquipment(newEq.ToJson().dump());

  UpdateEquipmentMessage msg;
  msg.data = newEq;
  msg.idx = GetIdx();
  for (auto listener : GetActorListeners()) {
    listener->SendToUser(msg, true);
  }
}

void MpActor::AddSpell(const uint32_t spellId)
{
  EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.learnedSpells.LearnSpell(spellId);
  });
}

void MpActor::RemoveSpell(const uint32_t spellId)
{
  EditChangeForm([&](MpChangeForm& changeForm) {
    changeForm.learnedSpells.ForgetSpell(spellId);
  });
}

void MpActor::SetLastAnimEvent(
  const std::optional<AnimationData>& animationData)
{
  pImpl->animationData = animationData;
}

std::optional<AnimationData> MpActor::GetLastAnimEvent() const
{
  return pImpl->animationData;
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

void MpActor::AddToFaction(Faction faction, bool lazyLoad)
{
  if (factionsLoaded == false && lazyLoad)
    LoadFactions();

  EditChangeForm([&](MpChangeFormREFR& changeForm) {
    if (!changeForm.factions.has_value()) {
      changeForm.factions = std::vector<Faction>();
      changeForm.factions.value().push_back(faction);
    } else {
      for (const auto& fact : changeForm.factions.value()) {
        if (faction.formDesc == fact.formDesc) {
          return;
        }
      }
      changeForm.factions.value().push_back(faction);
    }
  });
}

bool MpActor::IsInFaction(FormDesc factionForm, bool lazyLoad)
{
  if (factionsLoaded == false && lazyLoad)
    LoadFactions();

  const auto& factions = GetChangeForm().factions;

  if (!factions.has_value()) {
    return false;
  }

  for (const auto& faction : factions.value()) {
    if (faction.formDesc == factionForm) {
      return true;
    }
  }
  return false;
}

std::vector<Faction> MpActor::GetFactions(int minFactionRank,
                                          int maxFactionRank, bool lazyLoad)
{
  if (factionsLoaded == false && lazyLoad)
    LoadFactions();

  std::vector<Faction> result = std::vector<Faction>();

  if (minFactionRank < -128 || minFactionRank > 127 || maxFactionRank < -128 ||
      maxFactionRank > 127 || minFactionRank > maxFactionRank) {
    spdlog::warn(
      "Actor.GetFactions - minRank > maxRank or out of range (-128/127)");
    return result;
  }

  const auto& factions = GetChangeForm().factions;

  if (!factions.has_value()) {
    return result;
  }

  for (const auto& faction : factions.value()) {
    if (faction.rank >= minFactionRank && faction.rank <= maxFactionRank) {
      result.push_back(faction);
    }
  }

  return result;
}

void MpActor::RemoveFromFaction(FormDesc factionForm, bool lazyLoad)
{
  if (factionsLoaded == false && lazyLoad)
    LoadFactions();

  EditChangeForm([&](MpChangeFormREFR& changeForm) {
    if (!changeForm.factions.has_value()) {
      return;
    }

    auto& factions = changeForm.factions.value();
    factions.erase(std::remove_if(factions.begin(), factions.end(),
                                  [&](const Faction& faction) {
                                    return faction.formDesc == factionForm;
                                  }),
                   factions.end());
  });
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
    baseActorValues = GetBaseActorValues(worldState, baseId, raceId,
                                         ChangeForm().templateChain);
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

  if (!changeForm.templateChain.empty()) {
    // should be faster than nlohmann::json
    std::string jsonDump = "[";
    for (auto& element : changeForm.templateChain) {
      jsonDump += std::to_string(element.ToFormId(GetParent()->espmFiles));
      jsonDump += ',';
    }
    jsonDump.pop_back(); // comma
    jsonDump += "]";
    visitor("templateChain", jsonDump.data());
  }
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

  bool isTorch = false;
  if (espm::utils::Is<espm::LIGH>(recordType)) {
    auto& compressedFieldsCache = GetParent()->GetEspmCache();
    auto light = espm::Convert<espm::LIGH>(lookupRes.rec);
    auto res = light->GetData(compressedFieldsCache);
    isTorch = res.data.flags & espm::LIGH::Flags::CanBeCarried;
  }

  const bool isScroll = recordType == "SCRL";
  const bool isWeapon = recordType == "WEAP";
  const bool isArmor = recordType == "ARMO";
  const bool isAmmo = recordType == "AMMO";

  if (!(isSpell || isIngredient || isPotion || isBook || isTorch || isScroll ||
        isWeapon || isArmor || isAmmo)) {
    return false;
  }

  const bool hasItem = isSpell
    ? ChangeForm().learnedSpells.IsSpellLearned(baseId)
    : GetInventory().GetItemCount(baseId) > 0;

  if (!hasItem) {
    return false;
  }

  bool spellLearned = false;
  if (isIngredient || isPotion) {
    EatItem(baseId, recordType);

    nlohmann::json j = nlohmann::json::array();
    j.push_back(
      nlohmann::json({ { "formId", baseId },
                       { "type", isIngredient ? "Ingredient" : "Potion" } }));
    j.push_back(false);
    j.push_back(false);

    std::string serializedArgs = j.dump();
    for (auto listener : GetActorListeners()) {
      if (listener != this) {
        SpSnippet("Actor", "EquipItem", serializedArgs.data(), GetFormId())
          .Execute(listener, SpSnippetMode::kNoReturnResult);
      }
    }
  } else if (isBook) {
    spellLearned = ReadBook(baseId);
  }

  if (isIngredient || isPotion || spellLearned) {
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
        EnsureTemplateChainEvaluated(GetParent()->GetEspm(),
                                     Mode::NoRequestSave);
        changeForm.actorValues = GetBaseActorValues(
          GetParent(), GetBaseId(), GetRaceId(), changeForm.templateChain);
      }
    },
    Mode::NoRequestSave);
  ReapplyMagicEffects();

  // We do the same in PartOne::SetUserActor for player characters
  if (IsDead() && !IsRespawning()) {
    spdlog::info("MpActor::ApplyChangeForm {:x} - respawning dead actor",
                 GetFormId());
    RespawnWithDelay();
  }

  // Mirrors MpObjectReference impl
  // Perform all required grid operations
  newChangeForm.isDisabled ? Disable() : Enable();
  SetCellOrWorldObsolete(newChangeForm.worldOrCellDesc);
  SetPos(newChangeForm.position);
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
  message.data.health = actorValues.healthPercentage;
  message.data.magicka = actorValues.magickaPercentage;
  message.data.stamina = actorValues.staminaPercentage;
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

bool MpActor::IsSpellLearned(const uint32_t spellId) const
{
  return ChangeForm().learnedSpells.IsSpellLearned(spellId) ||
    IsSpellLearnedFromBase(spellId);
}

bool MpActor::IsSpellLearnedFromBase(const uint32_t spellId) const
{
  // TODO: support npc templates here?

  const auto npcData = espm::GetData<espm::NPC_>(GetBaseId(), GetParent());
  const auto npc = GetParent()->GetEspm().GetBrowser().LookupById(GetBaseId());

  const uint32_t raceId = npc.ToGlobalId(npcData.race);

  const auto raceData = espm::GetData<espm::RACE>(raceId, GetParent());
  const auto race = GetParent()->GetEspm().GetBrowser().LookupById(raceId);

  for (auto npcSpellRaw : npcData.spells) {
    const auto npcSpell = npc.ToGlobalId(npcSpellRaw);
    if (npcSpell == spellId) {
      return true;
    }
  }

  for (auto raceSpellRaw : raceData.spells) {
    const auto raceSpell = race.ToGlobalId(raceSpellRaw);
    if (raceSpell == spellId) {
      return true;
    }
  }

  return false;
}

std::vector<uint32_t> MpActor::GetSpellList() const
{
  return ChangeForm().learnedSpells.GetLearnedSpells();
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

namespace {
bool IsValidAnimEventName(const std::string& eventName)
{
  return std::all_of(eventName.begin(), eventName.end(),
                     [](char c) { return std::isalnum(c) || (c == '_'); });
}
}

std::string MpActor::GetLastAnimEventAsJson() const
{
  std::optional<AnimationData> anim = GetLastAnimEvent();

  if (!anim) {
    return "";
  }

  // Don't want bad anims to break JSON syntax
  if (!IsValidAnimEventName(anim->animEventName)) {
    return "";
  }

  std::string res = R"({"animEventName":)";
  res += '"' + anim->animEventName + '"';
  res += R"(,"numChanges":)";
  res += std::to_string(anim->numChanges);
  res += "}";
  return res;
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

  return EvaluateTemplate<espm::NPC_::UseTraits>(
    GetParent(), GetBaseId(), GetTemplateChain(),
    [](const auto& npcLookupResult, const auto& npcData) {
      return npcLookupResult.ToGlobalId(npcData.race);
    });
}

bool MpActor::IsWeaponDrawn() const
{
  return GetAnimationVariableBool("_skymp_isWeapDrawn");
}

espm::ObjectBounds MpActor::GetBounds() const
{
  return espm::GetData<espm::NPC_>(GetBaseId(), GetParent()).objectBounds;
}

const std::vector<FormDesc>& MpActor::GetTemplateChain() const
{
  return ChangeForm().templateChain;
}

bool MpActor::IsCreatedAsPlayer() const
{
  return GetFormId() >= 0xff000000 && GetBaseId() <= 0x7;
}

const ActorValues& MpActor::GetActorValues() const
{
  return ChangeForm().actorValues;
}

const ActiveMagicEffectsMap& MpActor::GetActiveMagicEffects() const
{
  return ChangeForm().activeMagicEffects;
}

int32_t MpActor::GetProfileId() const
{
  return ChangeForm().profileId;
}

void MpActor::SendAndSetDeathState(bool isDead, bool shouldTeleport)
{
  spdlog::trace(
    "MpActor::SendAndSetDeathState {:x} - isDead: {}, shouldTeleport: {}",
    GetFormId(), isDead, shouldTeleport);

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
    res.tChangeValues->data.health = kAttributePercentageFull;
    res.tChangeValues->data.magicka = kAttributePercentageFull;
    res.tChangeValues->data.stamina = kAttributePercentageFull;
  }

  return res;
}

void MpActor::EatItem(uint32_t baseId, espm::Type t)
{
  bool isIngredient = t == "INGR";
  bool isAlchemyItem = t == "ALCH";

  EatItemEvent eatItemEvent(this, baseId, isIngredient, isAlchemyItem);
  eatItemEvent.Fire(GetParent());
}

bool MpActor::ReadBook(const uint32_t baseId)
{
  ReadBookEvent readBookEvent(this, baseId);
  readBookEvent.Fire(GetParent());
  return readBookEvent.SpellLearned();
}

void MpActor::EnsureTemplateChainEvaluated(espm::Loader& loader,
                                           ChangeFormGuard::Mode mode)
{
  constexpr auto kPcLevel = 0;

  auto worldState = GetParent();
  if (!worldState) {
    return;
  }

  auto baseId = GetBaseId();
  if (baseId == 0x7 || baseId == 0) {
    return;
  }

  const auto& templateChain = ChangeForm().templateChain;

  if (!templateChain.empty()) {

    std::string errorTraits;
    std::string errorStats;
    std::string errorFactions;
    std::string errorSpelllist;
    std::string errorAIData;
    std::string errorAIPackages;
    std::string errorUnused;
    std::string errorBaseData;
    std::string errorInventory;
    std::string errorScript;
    std::string errorDefPackList;
    std::string errorAttackData;
    std::string errorKeywords;

    auto doNothing = [](const auto&, const auto&) { return 1; };

    EvaluateTemplateNoThrow<espm::NPC_::UseTraits>(
      worldState, baseId, templateChain, doNothing, &errorTraits);

    EvaluateTemplateNoThrow<espm::NPC_::UseStats>(
      worldState, baseId, templateChain, doNothing, &errorStats);

    EvaluateTemplateNoThrow<espm::NPC_::UseFactions>(
      worldState, baseId, templateChain, doNothing, &errorFactions);

    EvaluateTemplateNoThrow<espm::NPC_::UseSpelllist>(
      worldState, baseId, templateChain, doNothing, &errorSpelllist);

    EvaluateTemplateNoThrow<espm::NPC_::UseAIData>(
      worldState, baseId, templateChain, doNothing, &errorAIData);

    EvaluateTemplateNoThrow<espm::NPC_::UseAIPackages>(
      worldState, baseId, templateChain, doNothing, &errorAIPackages);

    EvaluateTemplateNoThrow<espm::NPC_::Unused>(
      worldState, baseId, templateChain, doNothing, &errorUnused);

    EvaluateTemplateNoThrow<espm::NPC_::UseBaseData>(
      worldState, baseId, templateChain, doNothing, &errorBaseData);

    EvaluateTemplateNoThrow<espm::NPC_::UseInventory>(
      worldState, baseId, templateChain, doNothing, &errorInventory);

    EvaluateTemplateNoThrow<espm::NPC_::UseScript>(
      worldState, baseId, templateChain, doNothing, &errorScript);

    EvaluateTemplateNoThrow<espm::NPC_::UseDefPackList>(
      worldState, baseId, templateChain, doNothing, &errorDefPackList);

    EvaluateTemplateNoThrow<espm::NPC_::UseAttackData>(
      worldState, baseId, templateChain, doNothing, &errorAttackData);

    EvaluateTemplateNoThrow<espm::NPC_::UseKeywords>(
      worldState, baseId, templateChain, doNothing, &errorKeywords);

    if (errorTraits.empty() && errorStats.empty() && errorFactions.empty() &&
        errorSpelllist.empty() && errorAIData.empty() &&
        errorAIPackages.empty() && errorUnused.empty() &&
        errorBaseData.empty() && errorInventory.empty() &&
        errorScript.empty() && errorDefPackList.empty() &&
        errorAttackData.empty() && errorKeywords.empty()) {
      return;
    }

    spdlog::info(
      "MpActor::EnsureTemplateChainEvaluate {:x} - One of EvaluateTemplate "
      "errored, forgetting previous template chain. Likely, an update on "
      "esp/esm side.",
      GetFormId());
  }

  EditChangeForm(
    [&](MpChangeFormREFR& changeForm) {
      auto headNpc = loader.GetBrowser().LookupById(baseId);
      std::vector<uint32_t> res = LeveledListUtils::EvaluateTemplateChain(
        loader.GetBrowser(), headNpc, kPcLevel);
      std::vector<FormDesc> templateChain(res.size());
      std::transform(
        res.begin(), res.end(), templateChain.begin(), [&](uint32_t formId) {
          return FormDesc::FromFormId(formId, worldState->espmFiles);
        });
      changeForm.templateChain = std::move(templateChain);
    },
    mode);
}

void MpActor::AddDeathItem()
{
  auto map = EvaluateDeathItem();
  for (auto& p : map) {
    AddItem(p.first, p.second);
  }
}

void MpActor::LoadFactions()
{
  std::vector<Faction> factions = EvaluateTemplate<espm::NPC_::UseFactions>(
    GetParent(), GetBaseId(), GetTemplateChain(),
    [&](const auto& npcLookupResult, const auto& npcData) {
      std::vector<Faction> factions = std::vector<Faction>();
      for (auto npcFaction : npcData.factions) {
        Faction faction = Faction();
        faction.formDesc =
          FormDesc::FromFormId(npcLookupResult.ToGlobalId(npcFaction.formId),
                               GetParent()->espmFiles);
        faction.rank = npcFaction.rank;
        factions.push_back(faction);
      }
      return factions;
    });
  for (Faction faction : factions) {
    AddToFaction(faction, false);
  }
  factionsLoaded = true;
}

std::map<uint32_t, uint32_t> MpActor::EvaluateDeathItem()
{
  auto worldState = GetParent();
  if (!worldState) {
    return {};
  }

  constexpr int kPlayerCharacterLevel = 1;

  auto& loader = worldState->GetEspm();

  auto base = loader.GetBrowser().LookupById(GetBaseId());
  if (!base.rec) {
    spdlog::error("EvaluateDeathItem {:x} - No base form", GetFormId());
    return {};
  }

  auto npc = espm::Convert<espm::NPC_>(base.rec);
  if (!npc) {
    spdlog::error(
      "EvaluateDeathItem {:x} - Expected base type to be NPC_, but got {}",
      GetFormId(), base.rec->GetType().ToString());
    return {};
  }

  uint32_t baseId = base.ToGlobalId(base.rec->GetId());
  auto& templateChain = ChangeForm().templateChain;

  uint32_t deathItemId = EvaluateTemplate<espm::NPC_::UseTraits>(
    worldState, baseId, templateChain,
    [](const auto& npcLookupResult, const auto& npcData) {
      return npcLookupResult.ToGlobalId(npcData.deathItem);
    });

  if (deathItemId == 0) {
    spdlog::info("EvaluateDeathItem {:x} - No death item found, skipping add",
                 GetFormId());
    return {};
  }

  espm::LookupResult deathItemLookupRes =
    loader.GetBrowser().LookupById(deathItemId);
  if (!deathItemLookupRes.rec) {
    spdlog::error("EvaluateDeathItem {:x} - Death item {:x} not found in espm",
                  GetFormId(), deathItemId);
    return {};
  }

  auto deathItemLvli = espm::Convert<espm::LVLI>(deathItemLookupRes.rec);
  if (!deathItemLvli) {
    spdlog::error("EvaluateDeathItem {:x} - Expected death item type to be "
                  "LVLI, but got {}",
                  GetFormId(), deathItemLookupRes.rec->GetType().ToString());
    return {};
  }

  const auto kCountMult = 1;
  auto map = LeveledListUtils::EvaluateListRecurse(
    loader.GetBrowser(), deathItemLookupRes, kCountMult,
    kPlayerCharacterLevel);
  return map;
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
    auto& espm = worldState->GetEspm();
    EnsureTemplateChainEvaluated(espm);
    EnsureBaseContainerAdded(espm); // template chain needed here

    // TODO: implement "gearedUpWeapons" flag
    EquipBestWeapon();
  }
}

void MpActor::Kill(MpActor* killer, bool shouldTeleport)
{
  spdlog::trace("MpActor::Kill {:x} - killer is {:x}", GetFormId(),
                killer ? killer->GetFormId() : 0);

  // Keep in sync with MpActor::SetIsDead
  SendAndSetDeathState(true, shouldTeleport);

  DeathEvent deathEvent(this, killer);
  deathEvent.Fire(GetParent());

  AddDeathItem();
}

void MpActor::RespawnWithDelay(bool shouldTeleport)
{
  spdlog::trace("MpActor::RespawnWithDelay {:x} - isRespawning: {}",
                GetFormId(), pImpl->isRespawning);

  if (pImpl->isRespawning) {
    return;
  }
  pImpl->isRespawning = true;

  ++pImpl->respawnTimerIndex;
  auto respawnTimerIndex = pImpl->respawnTimerIndex;

  uint32_t formId = GetFormId();
  if (auto worldState = GetParent()) {
    float respawnTime = GetRespawnTime();
    auto time = Viet::TimeUtils::To<std::chrono::milliseconds>(respawnTime);
    worldState->SetTimer(time).Then([worldState, this, formId, shouldTeleport,
                                     respawnTimerIndex,
                                     respawnTime](Viet::Void) {
      if (worldState->LookupFormById(formId).get() == this) {
        bool isLatestRespawn = respawnTimerIndex == pImpl->respawnTimerIndex;
        if (isLatestRespawn) {
          // This is implemented here, not in MpActor::Respawn because we don't
          // want inventory to be re-added in case of artificial resurrect from
          // the gamemode

          // This will do nothing if death item resolved to no items
          auto deathItem = EvaluateDeathItem();
          if (deathItem.empty()) {
            spdlog::info("MpActor::RespawnWithDelay {:x} - death item "
                         "resolved to no items, keeping inventory",
                         GetFormId());
          } else {
            spdlog::info("MpActor::RespawnWithDelay {:x} - death item "
                         "resolved to {} items, re-adding base inventory",
                         GetFormId(), deathItem.size());

            if (!worldState->HasEspm()) {
              spdlog::error(
                "MpActor::RespawnWithDelay {:x} - no espm attached",
                GetFormId());
            } else {
              Inventory inventory = GetInventory();
              Inventory inventoryToKeep;
              for (auto& entry : inventory.entries) {
                if (worldState->HasKeyword(entry.baseId, "SweetCantDrop")) {
                  inventoryToKeep.entries.push_back(entry);
                }
              }
              EditChangeForm(
                [&](MpChangeForm& changeForm) {
                  changeForm.inv = inventoryToKeep;
                  changeForm.baseContainerAdded = false;
                },
                Mode::NoRequestSave);
              EnsureBaseContainerAdded(worldState->GetEspm());
              spdlog::info("MpActor::RespawnWithDelay {:x} - {} inventory "
                           "entries with keyword kept",
                           GetFormId(), inventoryToKeep.entries.size());
            }
          }

          // Do actual respawn
          spdlog::info("MpActor::RespawnWithDelay {:x} - finally, respawn "
                       "after {} seconds",
                       GetFormId(), respawnTime);
          Respawn(shouldTeleport);
        }
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

  RespawnEvent respawnEvent(this, shouldTeleport);
  respawnEvent.Fire(GetParent());
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
  auto formId = GetFormId();

  if (!IsCreatedAsPlayer()) {
    if (formId < 0xff000000) {
      return GetEditorLocationalData();
    }
  }
  return ChangeForm().spawnPoint;
}

LocationalData MpActor::GetEditorLocationalData() const
{
  auto formId = GetFormId();
  auto worldState = GetParent();

  if (!worldState || !worldState->HasEspm()) {
    throw std::runtime_error("MpActor::GetEditorLocation can only be used "
                             "with actors attached to a valid world state");
  }

  auto data = espm::GetData<espm::ACHR>(formId, worldState);
  auto lookupRes = worldState->GetEspm().GetBrowser().LookupById(formId);

  // TODO: Angles are probably messed up here (radians/degrees)

  NiPoint3 pos;
  NiPoint3 rot;
  uint32_t worldOrCell = 0x0000003c;

  if (!lookupRes.rec) {
    spdlog::error("MpActor::GetEditorLocationalData {:x} - lookupRes.rec was "
                  "nullptr, using current location as spawn point",
                  formId);

    pos = this->GetPos();
    rot = this->GetAngle();
    worldOrCell = this->GetCellOrWorld().ToFormId(worldState->espmFiles);
  } else if (!data.loc) {
    spdlog::error("MpActor::GetEditorLocationalData {:x} - data.loc was "
                  "nullptr, using current location as spawn point",
                  formId);

    pos = this->GetPos();
    rot = this->GetAngle();
    worldOrCell = this->GetCellOrWorld().ToFormId(worldState->espmFiles);
  } else {
    pos = LocationalDataUtils::GetPos(data.loc);
    rot = LocationalDataUtils::GetRot(data.loc);
    worldOrCell = LocationalDataUtils::GetWorldOrCell(
      worldState->GetEspm().GetBrowser(), lookupRes);
  }

  return LocationalData{
    pos, rot, FormDesc::FromFormId(worldOrCell, worldState->espmFiles)
  };
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
  spdlog::trace("MpActor::SetIsDead {:x} - isDead: {}", GetFormId(), isDead);

  constexpr bool kShouldTeleport = false;

  if (isDead) {
    if (IsDead() == false) {

      // Keep in sync with MpActor::Kill
      SendAndSetDeathState(isDead, kShouldTeleport);

      DeathEvent deathEvent(this, nullptr);
      deathEvent.Fire(GetParent());

      AddDeathItem();

      spdlog::trace("MpActor::SetIsDead {:x} - actor is now dead",
                    GetFormId());
    } else {
      spdlog::trace("MpActor::SetIsDead {:x} - actor is already dead",
                    GetFormId());
    }
  } else {
    // same as SendAndSetDeathState but resets isRespawning flag
    Respawn(kShouldTeleport);
    spdlog::trace("MpActor::SetIsDead {:x} - actor is now alive", GetFormId());
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
  return GetBaseActorValues(GetParent(), GetBaseId(), GetRaceId(),
                            ChangeForm().templateChain);
}

BaseActorValues MpActor::GetMaximumValues()
{
  return GetBaseValues();
}

void MpActor::DropItem(const uint32_t baseId, const Inventory::Entry& entry)
{
  constexpr float kDeletionTimeSeconds = 2 * 60;
  constexpr size_t kDroppedItemsQueueMax = 10;

  static std::atomic<bool> g_dropItemDisabledGlobally = false;
  static std::atomic<int> g_numDrops = 0;

  if (g_dropItemDisabledGlobally) {
    return;
  }

  if (g_numDrops > 1'000'000) {
    spdlog::warn(
      "MpActor::DropItem - Too many item drops, server restart needed");
    return;
  }

  constexpr uint32_t kGold001 = 0x0000000f;
  if (baseId == kGold001) {
    spdlog::warn("MpActor::DropItem {:x} - Attempt to drop Gold001 by actor",
                 GetFormId());
    return;
  }

  const int count = entry.count;

  DropItemEvent dropEvent(GetFormId(), baseId, count);
  bool success = dropEvent.Fire(GetParent());

  if (!success) {
    spdlog::info("MpActor::DropItem {:x} - blocked by MpApiDropItem "
                 "(baseId={:x}, count={})",
                 GetFormId(), baseId, count);
    return;
  }

  auto worldState = GetParent();

  espm::LookupResult lookupRes =
    worldState->GetEspm().GetBrowser().LookupById(baseId);
  lookupRes.rec->GetId();

  std::string editorId =
    lookupRes.rec->GetEditorId(worldState->GetEspmCache());

  spdlog::trace("MpActor::DropItem - dropping {}", editorId);
  RemoveItems({ entry });

  PapyrusObjectReference papyrusObjectReference;
  auto baseForm = VarValue(std::make_shared<EspmGameObject>(lookupRes));
  auto aCount = VarValue(count);
  auto aForcePersist = VarValue(false);
  auto aInitiallyDisabled = VarValue(false);

  VarValue placedObjectWrap = papyrusObjectReference.PlaceAtMe(
    this->ToVarValue(),
    { baseForm, aCount, aForcePersist, aInitiallyDisabled });
  MpObjectReference* placedObject =
    GetFormPtr<MpObjectReference>(placedObjectWrap);

  if (!placedObject) {
    spdlog::error("MpActor::DropItem - placedObject was null, disabling "
                  "DropItem for all players");
    g_dropItemDisabledGlobally = true;
    return;
  }

  placedObject->SetCount(count);

  uint32_t droppedItemFormId = placedObject->GetFormId();

  // Filter our dropped items queue
  pImpl->droppedItemsQueue.erase(
    std::remove_if(
      pImpl->droppedItemsQueue.begin(), pImpl->droppedItemsQueue.end(),
      [worldState](const std::pair<uint32_t, MpObjectReference*>& pair) {
        auto [referenceFormId, reference] = pair;
        bool referenceAlive =
          reference == worldState->LookupFormById(referenceFormId).get();
        if (!referenceAlive) {
          return true;
        }
        if (reference->IsDeleted() || reference->IsHarvested()) {
          return true;
        }
        return false;
      }),
    pImpl->droppedItemsQueue.end());

  while (!pImpl->droppedItemsQueue.empty() &&
         pImpl->droppedItemsQueue.size() >= kDroppedItemsQueueMax) {
    auto [referenceFormId, reference] = pImpl->droppedItemsQueue.front();
    bool referenceAlive =
      reference == worldState->LookupFormById(referenceFormId).get();
    if (referenceAlive) {
      if (!reference->IsDeleted()) {
        spdlog::trace("MpActor::DropItem - deleting previously dropped {}",
                      editorId);
        reference->Delete();
      } else {
        spdlog::warn("MpActor::DropItem - reference in queue was deleted");
      }
    } else {
      spdlog::warn("MpActor::DropItem - reference in queue was invalidated");
    }
    pImpl->droppedItemsQueue.erase(pImpl->droppedItemsQueue.begin());
  }

  pImpl->droppedItemsQueue.push_back(
    std::make_pair(droppedItemFormId, placedObject));

  auto time =
    Viet::TimeUtils::To<std::chrono::milliseconds>(kDeletionTimeSeconds);

  uint32_t formId = GetFormId();

  // TODO: make timer group for better performance
  worldState->SetTimer(time).Then([placedObject, worldState, droppedItemFormId,
                                   editorId, this, formId](Viet::Void) {
    bool actorStillAlive = this == worldState->LookupFormById(formId).get();
    if (!actorStillAlive) {
      return;
    }
    bool formStillAlive =
      placedObject == worldState->LookupFormById(droppedItemFormId).get();
    if (!formStillAlive) {
      return;
    }

    if (placedObject->IsDeleted()) {
      return;
    }

    spdlog::trace("MpActor::DropItem - deleting previously dropped {}",
                  editorId);

    // Item deleted, not in queue anymore
    auto it = std::remove(this->pImpl->droppedItemsQueue.begin(),
                          this->pImpl->droppedItemsQueue.end(),
                          std::make_pair(droppedItemFormId, placedObject));
    this->pImpl->droppedItemsQueue.erase(it,
                                         this->pImpl->droppedItemsQueue.end());

    placedObject->Delete();
    ++g_numDrops;
  });
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

  if (data.effectType == espm::MGEF::EffectType::CureDisease) {
    spdlog::trace("Curing all diseases");
    auto spells = ChangeForm().learnedSpells.GetLearnedSpells();
    for (auto spellId : spells) {
      auto spellData = espm::GetData<espm::SPEL>(spellId, worldState);
      if (spellData.spellItem->type == espm::SPEL::SpellType::Disease) {
        spdlog::trace("Curing disease {:x}", spellId);
        RemoveSpell(spellId);
      }
    }
    return;
  }

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
      // this coefficient (workaround) has been added for sake of game
      // balance and because of disability to restrict players use potions
      // often on client side
      constexpr float kMagnitudeCoeff = 100.f;
      RestoreActorValuePatched(this, av, effect.magnitude * kMagnitudeCoeff);
    } else {
      RestoreActorValuePatched(this, av, effect.magnitude);
    }
  }

  if (isRate || isMult) {
    MpChangeForm changeForm = GetChangeForm();
    BaseActorValues baseValues = GetBaseActorValues(
      GetParent(), GetBaseId(), GetRaceId(), changeForm.templateChain);
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
      // TODO: proper fix (or effects system) instead of monkey-patching 4x
      // higher mult
      // https://github.com/skyrim-multiplayer/skymp/pull/1852
      spdlog::trace("The result of baseValue * mult is: {}*{}={}", baseValue,
                    mult, baseValue * (mult * 4));
      SetActorValue(av, baseValue * (mult * 4));
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
  const ActorValues baseActorValues = GetBaseActorValues(
    GetParent(), GetBaseId(), GetRaceId(), ChangeForm().templateChain);
  const float baseActorValue = baseActorValues.GetValue(actorValue);
  SetActorValue(actorValue, baseActorValue);
  EditChangeForm([actorValue](MpChangeForm& changeForm) {
    changeForm.activeMagicEffects.Remove(actorValue);
  });
}

void MpActor::RemoveAllMagicEffects() noexcept
{
  const ActorValues baseActorValues = GetBaseActorValues(
    GetParent(), GetBaseId(), GetRaceId(), ChangeForm().templateChain);
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
    if (entry.GetWorn() != Inventory::Worn::None) {
      espm::LookupResult res = espmBrowser.LookupById(entry.baseId);
      auto* weaponRecord = espm::Convert<espm::WEAP>(res.rec);
      if (weaponRecord) {
        if (entry.GetWorn() == Inventory::Worn::Left) {
          wornWeaponEntries[0] = std::move(entry);
        }
        if (entry.GetWorn() == Inventory::Worn::Right) {
          wornWeaponEntries[1] = std::move(entry);
        }
      }
    }
  }
  return wornWeaponEntries;
}
