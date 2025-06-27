#include "ActionListener.h"
#include "AnimationSystem.h"
#include "ConditionsEvaluator.h"
#include "ConsoleCommands.h"
#include "CropRegeneration.h"
#include "Exceptions.h"
#include "GetBaseActorValues.h"
#include "HitData.h"
#include "MathUtils.h"
#include "MovementValidation.h"
#include "MpObjectReference.h"
#include "MsgType.h"
#include "Overloaded.h"
#include "WorldState.h"
#include "gamemode_events/CustomEvent.h"
#include "gamemode_events/EatItemEvent.h"
#include "gamemode_events/UpdateAppearanceAttemptEvent.h"
#include "gamemode_events/UpdateEquipmentAttemptEvent.h"
#include "script_objects/EspmGameObject.h"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <unordered_set>

#include "HostStartMessage.h"
#include "HostStopMessage.h"
#include "UpdateEquipmentMessage.h"
#include "SpSnippet.h"
#include "UpdateAnimVariablesMessage.h"
#include "CustomPacketMessage.h"

MpActor* ActionListener::SendToNeighbours(uint32_t idx,
                                          Networking::UserId userId,
                                          Networking::PacketData data,
                                          size_t length, bool reliable)
{
  MpActor* myActor = partOne.serverState.ActorByUser(userId);
  // The old behavior is doing nothing in that case. This is covered by tests
  if (!myActor) {
    spdlog::warn("SendToNeighbours - No actor assigned to user");
    return nullptr;
  }

  MpForm* form = partOne.worldState.LookupFormByIdx(idx);
  MpActor* actor = form ? form->AsActor() : nullptr;
  if (!actor) {
    spdlog::error("SendToNeighbours - Target actor doesn't exist");
    return nullptr;
  }

  if (idx != myActor->GetIdx()) {
    // Possible fix for "players link to each other" bug
    // See also PartOne::SetUserActor
    Networking::UserId actorsOwningUserId =
      partOne.serverState.UserByActor(actor);
    if (actorsOwningUserId != Networking::InvalidUserId) {
      spdlog::error("SendToNeighbours - No permission to update actor {:x} "
                    "(already owned by user {})",
                    actor->GetFormId(), actorsOwningUserId);
      partOne.SendHostStop(userId, *actor);

      partOne.worldState.hosters.erase(actor->GetFormId());
      return nullptr;
    }

    auto it = partOne.worldState.hosters.find(actor->GetFormId());
    if (it == partOne.worldState.hosters.end() ||
        it->second != myActor->GetFormId()) {
      if (idx == 0) {
        spdlog::warn("SendToNeighbours - idx=0, <Message>::ReadJson or "
                     "similar is probably incorrect");
      }
      spdlog::error(
        "SendToNeighbours - No permission to update actor {:x} (not a hoster)",
        actor->GetFormId());
      partOne.SendHostStop(userId, *actor);
      return nullptr;
    }
  }

  for (auto listener : actor->GetActorListeners()) {
    auto targetuserId = partOne.serverState.UserByActor(listener);
    if (targetuserId != Networking::InvalidUserId) {
      partOne.GetSendTarget().Send(targetuserId, data, length, reliable);
    }
  }

  return actor;
}

MpActor* ActionListener::SendToNeighbours(uint32_t idx,
                                          const RawMessageData& rawMsgData,
                                          bool reliable)
{
  return SendToNeighbours(idx, rawMsgData.userId, rawMsgData.unparsed,
                          rawMsgData.unparsedLength, reliable);
}

void ActionListener::OnCustomPacket(const RawMessageData& rawMsgData, const CustomPacketMessage& msg)
{
  simdjson::dom::parser parser;
  auto content = parser.parse(msg.contentJsonDump).value();
  for (auto& listener : partOne.GetListeners()) {
    listener->OnCustomPacket(rawMsgData.userId, content);
  }
}

void ActionListener::OnUpdateMovement(const RawMessageData& rawMsgData, const UpdateMovementMessage& msg)
{
  auto actor = SendToNeighbours(msg.idx, rawMsgData);
  if (actor) {
    bool teleportFlag = actor->GetTeleportFlag();
    actor->SetTeleportFlag(false);

    static const NiPoint3 kInfinityPos = {
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity()
    };

    auto& espmFiles = actor->GetParent()->espmFiles;

    const auto& currentPos = actor->GetPos();
    const auto& currentRot = actor->GetAngle();
    const auto& currentCellOrWorld = actor->GetCellOrWorld();

    // if (!MovementValidation::Validate(
    //       currentPos, currentRot, currentCellOrWorld,
    //       teleportFlag ? reallyWrongPos : NiPoint3{msg.data.pos[0], msg.data.pos[1], msg.data.pos[2]},
    //       FormDesc::FromFormId(msg.data.worldOrCell, espmFiles),
    //       isMe ? static_cast<IMessageOutput&>(msgOutput)
    //            : static_cast<IMessageOutput&>(msgOutputDummy),
    //       espmFiles)) {
    //       partOne, currentPos, currentRot, currentCellOrWorld,
    //       teleportFlag ? kInfinityPos : pos,
    //       FormDesc::FromFormId(worldOrCell, espmFiles), rawMsgData.userId,
    //       actor, espmFiles)) {
    //   return;
    // }

    if (!MovementValidation::Validate(partOne, currentPos, currentRot, currentCellOrWorld, teleportFlag ? kInfinityPos : NiPoint3{msg.data.pos[0], msg.data.pos[1], msg.data.pos[2]}, 
        FormDesc::FromFormId(msg.data.worldOrCell, espmFiles), rawMsgData.userId, actor, espmFiles)) {
      return;
    }

    if (!msg.data.isBlocking) {
      actor->IncreaseBlockCount();
    } else {
      actor->ResetBlockCount();
    }

    actor->SetPos(NiPoint3{msg.data.pos[0], msg.data.pos[1], msg.data.pos[2]}, SetPosMode::CalledByUpdateMovement);
    actor->SetAngle(NiPoint3{msg.data.rot[0], msg.data.rot[1], msg.data.rot[2]}, SetAngleMode::CalledByUpdateMovement);
    actor->SetAnimationVariableBool(AnimationVariableBool::kVariable_bInJumpState, msg.data.isInJumpState);
    actor->SetAnimationVariableBool(AnimationVariableBool::kVariable__skymp_isWeapDrawn, msg.data.isWeapDrawn);
    actor->SetAnimationVariableBool(AnimationVariableBool::kVariable_IsBlocking, msg.data.isBlocking);

    if (actor->GetBlockCount() == 5) {
      actor->SetIsBlockActive(false);
      actor->ResetBlockCount();
    }

    if (msg.data.runMode != "Standing") {
      actor->SetLastAnimEvent(std::nullopt);
    }

    if (partOne.worldState.lastMovUpdateByIdx.size() <= msg.idx) {
      auto newSize = static_cast<size_t>(msg.idx) + 1;
      partOne.worldState.lastMovUpdateByIdx.resize(newSize);
    }
    partOne.worldState.lastMovUpdateByIdx[msg.idx] = std::chrono::system_clock::now();
  }
}

void ActionListener::OnUpdateAnimation(const RawMessageData& rawMsgData, const UpdateAnimationMessage& msg)
{
  MpActor* myActor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!myActor) {
    return;
  }

  auto targetActor = SendToNeighbours(msg.idx, rawMsgData);

  if (!targetActor) {
    return;
  }

  // Only process animation system and set last anim event for player's actor
  if (targetActor != myActor) {
    return;
  }

  partOne.animationSystem.Process(targetActor, msg.data);
  targetActor->SetLastAnimEvent(msg.data);
}

void ActionListener::OnUpdateAppearance(const RawMessageData& rawMsgData, const UpdateAppearanceMessage& msg)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor || !msg.data.has_value()) {
    return;
  }

  const bool isAllowed = actor->IsRaceMenuOpen();

  if (isAllowed) {
    actor->SetRaceMenuOpen(false);
    actor->SetAppearance(&msg.data.value());
    SendToNeighbours(msg.idx, rawMsgData, true);
  }

  UpdateAppearanceAttemptEvent updateAppearanceAttemptEvent(actor, msg.data.value(), isAllowed);
  updateAppearanceAttemptEvent.Fire(actor->GetParent());
}

void ActionListener::OnUpdateEquipment(const RawMessageData& rawMsgData, const UpdateEquipmentMessage& msg)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor) {
    return;
  }

  bool isAllowed = true;
  const Equipment& data = msg.data;
  const Inventory& equipmentInv = data.inv;
  uint32_t leftSpell = data.leftSpell.value_or(0);
  uint32_t rightSpell = data.rightSpell.value_or(0);
  uint32_t voiceSpell = data.voiceSpell.value_or(0);
  uint32_t instantSpell = data.instantSpell.value_or(0);

  if (leftSpell > 0 && !actor->IsSpellLearned(leftSpell)) {
    spdlog::debug("OnUpdateEquipment result false. Spell with id ({}) not learned", leftSpell);
    isAllowed = false;
  }
  if (rightSpell > 0 && !actor->IsSpellLearned(rightSpell)) {
    spdlog::debug("OnUpdateEquipment result false. Spell with id ({}) not learned", rightSpell);
    isAllowed = false;
  }
  if (voiceSpell > 0 && !actor->IsSpellLearned(voiceSpell)) {
    spdlog::debug("OnUpdateEquipment result false. Spell with id ({}) not learned", voiceSpell);
    isAllowed = false;
  }
  if (instantSpell > 0 && !actor->IsSpellLearned(instantSpell)) {
    spdlog::debug("OnUpdateEquipment result false. Spell with id ({}) not learned", instantSpell);
    isAllowed = false;
  }
  const auto& inventory = actor->GetInventory();
  for (auto& entry : equipmentInv.entries) {
    if (!inventory.HasItem(entry.baseId)) {
      spdlog::debug("OnUpdateEquipment result false. The inventory does not contain item with id {:x}", entry.baseId);
      isAllowed = false;
      break;
    }
  }
  if (isAllowed) {
    SendToNeighbours(msg.idx, rawMsgData, true);
    actor->SetEquipment(data);
  }
  UpdateEquipmentAttemptEvent updateEquipmentAttemptEvent(actor, data, isAllowed);
  updateEquipmentAttemptEvent.Fire(actor->GetParent());
}

void ActionListener::OnActivate(const RawMessageData& rawMsgData, const ActivateMessage& msg)
{
  if (!partOne.HasEspm())
    throw std::runtime_error("No loaded esm or esp files are found");

  const auto ac = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!ac)
    throw std::runtime_error("Can't do this without Actor attached");

  auto it = partOne.worldState.hosters.find(static_cast<uint32_t>(msg.data.caster));
  auto hosterId = it == partOne.worldState.hosters.end() ? 0 : it->second;

  if (msg.data.caster != 0x14) {
    if (hosterId != ac->GetFormId()) {
      std::stringstream ss;
      ss << std::hex << "Bad hoster is attached to caster 0x" << msg.data.caster
         << ", expected 0x" << ac->GetFormId() << ", but found 0x" << hosterId;
      throw std::runtime_error(ss.str());
    }
  }

  auto targetPtr = std::dynamic_pointer_cast<MpObjectReference>(
    partOne.worldState.LookupFormById(static_cast<uint32_t>(msg.data.target)));
  if (!targetPtr)
    return;

  constexpr bool kDefaultProcessingOnlyFalse = false;
  targetPtr->Activate(
    msg.data.caster == 0x14 ? *ac
                   : partOne.worldState.GetFormAt<MpObjectReference>(static_cast<uint32_t>(msg.data.caster)),
    kDefaultProcessingOnlyFalse, msg.data.isSecondActivation);
  if (hosterId) {
    auto actor = std::dynamic_pointer_cast<MpActor>(
      partOne.worldState.LookupFormById(static_cast<uint32_t>(msg.data.caster)));
    if (actor) {
      actor->EquipBestWeapon();
    }
  }
}

void ActionListener::OnPutItem(const RawMessageData& rawMsgData, const PutItemMessage& msg)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(msg.target);
  if (!actor)
    return;
  auto worldState = actor->GetParent();
  if (!worldState) {
    return spdlog::error("No WorldState attached");
  }
  if (worldState->HasKeyword(msg.baseId, "SweetCantDrop")) {
    return spdlog::error("Attempt to put SweetCantDrop item {:x}", actor->GetFormId());
  }
  Inventory::Entry entry;
  entry.baseId = msg.baseId;
  entry.count = msg.count;
  static_cast<Inventory::ExtraData&>(entry) = static_cast<const Inventory::ExtraData&>(msg);
  entry.SetWorn(Inventory::Worn::None);
  ref.PutItem(*actor, entry);
}

void ActionListener::OnTakeItem(const RawMessageData& rawMsgData, const TakeItemMessage& msg)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(msg.target);
  if (!actor)
    return;
  auto worldState = actor->GetParent();
  if (!worldState) {
    return spdlog::error("No WorldState attached");
  }
  if (worldState->HasKeyword(msg.baseId, "SweetCantDrop")) {
    return spdlog::error("Attempt to take SweetCantDrop item {:x}", actor->GetFormId());
  }
  Inventory::Entry entry;
  entry.baseId = msg.baseId;
  entry.count = msg.count;
  static_cast<Inventory::ExtraData&>(entry) = static_cast<const Inventory::ExtraData&>(msg);
  ref.TakeItem(*actor, entry);
}

void ActionListener::OnDropItem(const RawMessageData& rawMsgData, const DropItemMessage& msg)
{
  MpActor* ac = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!ac) {
    return spdlog::error("Unable to drop an item from user with id: {}.", rawMsgData.userId);
  }
  auto worldState = ac->GetParent();
  if (!worldState) {
    return spdlog::error("No WorldState attached");
  }
  if (worldState->HasKeyword(static_cast<uint32_t>(msg.baseId), "SweetCantDrop")) {
    return spdlog::error("Attempt to drop SweetCantDrop item {:x}", ac->GetFormId());
  }
  Inventory::Entry entry;
  entry.baseId = static_cast<uint32_t>(msg.baseId);
  entry.count = msg.count;
  ac->DropItem(static_cast<uint32_t>(msg.baseId), entry);
}

void ActionListener::OnPlayerBowShot(const RawMessageData& rawMsgData, const PlayerBowShotMessage& msg)
{
  MpActor* ac = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!ac) {
    return spdlog::error("Unable to shot from user with id: {}.", rawMsgData.userId);
  }
  auto worldState = ac->GetParent();
  if (!worldState) {
    return;
  }
  auto ammoLookupRes = worldState->GetEspm().GetBrowser().LookupById(msg.ammoId);
  if (!ammoLookupRes.rec) {
    return spdlog::error("ActionListener::OnPlayerBowShot {:x} - unable to find espm record for {:x}", ac->GetFormId(), msg.ammoId);
  }
  if (ammoLookupRes.rec->GetType().ToString() != "AMMO") {
    return spdlog::error("ActionListener::OnPlayerBowShot {:x} - unable to shot not an ammo {:x}", ac->GetFormId(), msg.ammoId);
  }
  ac->RemoveItem(msg.ammoId, 1, nullptr);
}

void ActionListener::OnFinishSpSnippet(const RawMessageData& rawMsgData, const FinishSpSnippetMessage& msg)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor)
    throw std::runtime_error(
      "Unable to finish SpSnippet: No Actor found for user " +
      std::to_string(rawMsgData.userId));
  actor->ResolveSnippet(static_cast<uint32_t>(msg.snippetIdx), SpSnippet::VarValueFromSpSnippetReturnValue(msg.returnValue));
}

void ActionListener::OnEquip(const RawMessageData& rawMsgData, const OnEquipMessage& msg)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor) {
    throw std::runtime_error(
      "Unable to finish SpSnippet: No Actor found for user " +
      std::to_string(rawMsgData.userId));
  }
  std::ignore = actor->OnEquip(msg.baseId);
}

void ActionListener::OnConsoleCommand(const RawMessageData& rawMsgData, const ConsoleCommandMessage& msg)
{
  MpActor* me = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (me) {
    std::vector<ConsoleCommands::Argument> consoleArgs;
    consoleArgs.resize(msg.data.args.size());
    for (size_t i = 0; i < msg.data.args.size(); i++) {
      consoleArgs[i] = ConsoleCommands::Argument(msg.data.args[i]);
    }
    ConsoleCommands::Execute(*me, msg.data.commandName, consoleArgs);
  }
}

void ActionListener::OnCraftItem(const RawMessageData& rawMsgData, const CraftItemMessage& msg)
{
  craftService->OnCraftItem(rawMsgData, msg.data.craftInputObjects, msg.data.workbench, msg.data.resultObjectId);
}

void ActionListener::OnHostAttempt(const RawMessageData& rawMsgData, const HostMessage& msg)
{
  MpActor* me = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!me) {
    throw std::runtime_error("Unable to host without actor attached");
  }
  auto& remote = partOne.worldState.GetFormAt<MpObjectReference>(static_cast<uint32_t>(msg.remoteId));
  auto user = partOne.serverState.UserByActor(remote.AsActor());
  if (user != Networking::InvalidUserId) {
    return;
  }
  auto& hoster = partOne.worldState.hosters[static_cast<uint32_t>(msg.remoteId)];
  const uint32_t prevHoster = hoster;
  auto remoteIdx = remote.GetIdx();
  std::optional<std::chrono::system_clock::time_point> lastRemoteUpdate;
  if (partOne.worldState.lastMovUpdateByIdx.size() > remoteIdx) {
    lastRemoteUpdate = partOne.worldState.lastMovUpdateByIdx[remoteIdx];
  }
  const auto hostResetTimeout = std::chrono::seconds(2);
  if (hoster == 0 || !lastRemoteUpdate || std::chrono::system_clock::now() - *lastRemoteUpdate > hostResetTimeout) {
    partOne.GetLogger().info("Hoster changed from {0:x} to {0:x}", prevHoster, me->GetFormId());
    hoster = me->GetFormId();
    remote.UpdateHoster(hoster);
    partOne.worldState.lastMovUpdateByIdx[remoteIdx] = std::chrono::system_clock::now();
    auto remoteAsActor = remote.AsActor();
    if (remoteAsActor) {
      remoteAsActor->EquipBestWeapon();
    }
    uint64_t longFormId = remote.GetFormId();
    if (remoteAsActor && longFormId < 0xff000000) {
      longFormId += 0x100000000;
    }

    HostStartMessage message;
    message.target = longFormId;
    partOne.GetSendTarget().Send(rawMsgData.userId, message, true);

    // Otherwise, health percentage would remain unsynced until someone hits
    // npc
    auto formId = remote.GetFormId();
    partOne.worldState.SetTimer(std::chrono::seconds(1)).Then([this, formId](Viet::Void) {
      auto& remote = partOne.worldState.GetFormAt<MpActor>(formId);
      auto changeForm = remote.GetChangeForm();
      ChangeValuesMessage msg;
      msg.idx = remote.GetIdx();
      msg.data.health = changeForm.actorValues.healthPercentage;
      msg.data.magicka = changeForm.actorValues.magickaPercentage;
      msg.data.stamina = changeForm.actorValues.staminaPercentage;
      remote.SendToUser(msg, true);
    });
    auto& prevHosterForm = partOne.worldState.LookupFormById(prevHoster);
    if (MpActor* prevHosterActor = prevHosterForm ? prevHosterForm->AsActor() : nullptr) {
      auto prevHosterUser = partOne.serverState.UserByActor(prevHosterActor);
      if (prevHosterUser != Networking::InvalidUserId && prevHosterUser != rawMsgData.userId) {
        HostStopMessage message;
        message.target = longFormId;
        partOne.GetSendTarget().Send(prevHosterUser, message, true);
      }
    }
  }
}

void ActionListener::OnCustomEvent(const RawMessageData& rawMsgData, const CustomEventMessage& msg)
{
  auto ac = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!ac) {
    return;
  }
  if (msg.eventName.empty() || msg.eventName[0] != '_') {
    return;
  }

  nlohmann::json jsonArray = nlohmann::json::array();

  for (auto& arg : msg.argsJsonDumps) {
    jsonArray.push_back(nlohmann::json::parse(arg));
  }

  const std::string jsonArrayDump = jsonArray.dump();

  for (auto& listener : partOne.GetListeners()) {
    CustomEvent customEvent(ac->GetFormId(), msg.eventName, jsonArrayDump);
    listener->OnMpApiEvent(customEvent);
  }
}

void ActionListener::OnChangeValues(const RawMessageData& rawMsgData, const ChangeValuesMessage& msg)
{
  // TODO: support partial updates
  if (!msg.data.health.has_value() || !msg.data.magicka.has_value() ||
      !msg.data.stamina.has_value()) {
    const std::string healthStr = msg.data.health.has_value()
      ? std::to_string(*msg.data.health)
      : "null";
    const std::string magickaStr = msg.data.magicka.has_value()
      ? std::to_string(*msg.data.magicka)
      : "null";
    const std::string staminaStr = msg.data.stamina.has_value()
      ? std::to_string(*msg.data.stamina)
      : "null";

    spdlog::error("ActionListener::OnChangeValues - health, magicka or "
                  "stamina is null {} {} {}",
                  healthStr, magickaStr, staminaStr);
    return;
  }

  // TODO: refactor our ActorValues struct
  ActorValues newActorValues;
  newActorValues.healthPercentage = *msg.data.health;
  newActorValues.magickaPercentage = *msg.data.magicka;
  newActorValues.staminaPercentage = *msg.data.stamina;

  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor) {
    return spdlog::error(
      "ActionListener::OnChangeValues - no Actor attached to userId {}",
      rawMsgData.userId);
  }

  if (actor->ShouldSkipRestoration()) {
    return;
  }

  auto now = std::chrono::steady_clock::now();

  float timeAfterRegeneration = CropPeriodAfterLastRegen(
    actor->GetDurationOfAttributesPercentagesUpdate(now).count());

  ActorValues currentActorValues = actor->GetActorValues();
  const float health = newActorValues.healthPercentage;
  const float magicka = newActorValues.magickaPercentage;
  const float stamina = newActorValues.staminaPercentage;

  const bool healthChanged =
    !MathUtils::IsNearlyEqual(currentActorValues.healthPercentage, health);
  const bool magickaChanged =
    !MathUtils::IsNearlyEqual(currentActorValues.magickaPercentage, magicka);
  const bool staminaChanged =
    !MathUtils::IsNearlyEqual(currentActorValues.staminaPercentage, stamina);

  if (healthChanged) {
    currentActorValues.healthPercentage =
      CropHealthRegeneration(health, timeAfterRegeneration, actor);
  }
  if (magickaChanged) {
    currentActorValues.magickaPercentage =
      CropMagickaRegeneration(magicka, timeAfterRegeneration, actor);
  }
  if (staminaChanged) {
    currentActorValues.staminaPercentage =
      CropStaminaRegeneration(stamina, timeAfterRegeneration, actor);
  }

  if (!MathUtils::IsNearlyEqual(currentActorValues.healthPercentage,
                                newActorValues.healthPercentage) ||
      !MathUtils::IsNearlyEqual(currentActorValues.magickaPercentage,
                                newActorValues.magickaPercentage) ||
      !MathUtils::IsNearlyEqual(currentActorValues.staminaPercentage,
                                newActorValues.staminaPercentage)) {

    std::vector<espm::ActorValue> avFilter;
    if (healthChanged) {
      avFilter.push_back(espm::ActorValue::Health);
    }
    if (magickaChanged) {
      avFilter.push_back(espm::ActorValue::Magicka);
    }
    if (staminaChanged) {
      avFilter.push_back(espm::ActorValue::Stamina);
    }
    actor->NetSendChangeValues(currentActorValues, avFilter);
  }
  actor->SetPercentages(currentActorValues);
}

namespace {

bool IsUnarmedAttack(const uint32_t sourceFormId)
{
  return sourceFormId == 0x1f4;
}

float CalculateCurrentHealthPercentage(const MpActor& actor, float damage,
                                       float healthPercentage,
                                       float* outBaseHealth)
{
  const uint32_t baseId = actor.GetBaseId();
  const uint32_t raceId = actor.GetRaceId();
  WorldState* espmProvider = actor.GetParent();

  const float baseHealth =
    GetBaseActorValues(espmProvider, baseId, raceId, actor.GetTemplateChain())
      .health;

  if (outBaseHealth) {
    *outBaseHealth = baseHealth;
  }

  const float damagePercentage = damage / baseHealth;
  const float currentHealthPercentage = healthPercentage - damagePercentage;

  /// TODO add check for nan and inf!
  return currentHealthPercentage <= 0.f ? 0.f : currentHealthPercentage;
}

float GetReach(const MpActor& actor, const uint32_t source,
               float reachHotfixMult)
{
  auto espmProvider = actor.GetParent();
  if (IsUnarmedAttack(source)) {
    uint32_t raceId = actor.GetRaceId();
    return reachHotfixMult *
      espm::GetData<espm::RACE>(raceId, espmProvider).unarmedReach;
  }
  auto weapDNAM = espm::GetData<espm::WEAP>(source, espmProvider).weapDNAM;
  float fCombatDistance =
    espm::GetData<espm::GMST>(espm::GMST::kFCombatDistance, espmProvider)
      .value;
  float weaponReach = weapDNAM ? weapDNAM->reach : 0;
  return reachHotfixMult * weaponReach * fCombatDistance;
}

NiPoint3 RotateZ(const NiPoint3& point, float angle)
{
  static const float kPi = std::acos(-1.f);
  static const float kAngleToRadians = kPi / 180.f;
  float cos = std::cos(angle * kAngleToRadians);
  float sin = std::sin(angle * kAngleToRadians);

  return { point.x * cos - point.y * sin, point.x * sin + point.y * cos,
           point.z };
}

float GetSqrDistanceToBounds(const MpActor& actor, const MpActor& target)
{
  // TODO(#491): Figure out where to take the missing reach component
  constexpr float kPatch = 15.f;

  auto bounds = actor.GetBounds();
  auto targetBounds = target.GetBounds();

  // "Y" is "face" of character
  const float angleZ = 90.f - target.GetAngle().z;
  float direction = actor.GetAngle().z;

  // vector from target to the actor
  NiPoint3 position = actor.GetPos() - target.GetPos();
  position += RotateZ(
    NiPoint3(kPatch + bounds.pos2[1], 0.f, 0.f + bounds.pos2[2]), direction);

  NiPoint3 pos = RotateZ(position, angleZ);

  bool isProjectionInside[3] = {
    (targetBounds.pos1[0] <= pos.x && pos.x <= targetBounds.pos2[0]),
    (targetBounds.pos1[1] <= pos.y && pos.y <= targetBounds.pos2[1]),
    (targetBounds.pos1[2] <= pos.z && pos.z <= targetBounds.pos2[2])
  };

  NiPoint3 nearestCorner = {
    pos[0] > 0 ? 0.f + targetBounds.pos2[0] : 0.f + targetBounds.pos1[0],
    pos[1] > 0 ? 0.f + targetBounds.pos2[1] : 0.f + targetBounds.pos1[1],
    pos[2] > 0 ? 0.f + targetBounds.pos2[2] : 0.f + targetBounds.pos1[2]
  };

  return NiPoint3(isProjectionInside[0] ? 0.f : pos.x - nearestCorner.x,
                  isProjectionInside[1] ? 0.f : pos.y - nearestCorner.y,
                  isProjectionInside[2] ? 0.f : pos.z - nearestCorner.z)
    .SqrLength();
}

bool IsBowOrCrossbowShot(const HitData& hitData, WorldState* worldState)
{
  if (!worldState || !worldState->HasEspm()) {
    return false;
  }

  if (hitData.isBashAttack) {
    return false;
  }

  auto sourceLookupRes =
    worldState->GetEspm().GetBrowser().LookupById(hitData.source);
  if (!sourceLookupRes.rec) {
    return false;
  }

  auto source = espm::Convert<espm::WEAP>(sourceLookupRes.rec);
  if (!source) {
    return false;
  }

  auto weapDNAM = source->GetData(worldState->GetEspmCache()).weapDNAM;

  if (weapDNAM->animType != espm::WEAP::AnimType::Bow &&
      weapDNAM->animType != espm::WEAP::AnimType::Crossbow) {
    return false;
  }

  return true;
}

bool IsDistanceValid(const MpActor& actor, const MpActor& targetActor,
                     const HitData& hitData)
{
  float sqrDistance = GetSqrDistanceToBounds(actor, targetActor);

  // TODO: fix bounding boxes for creatures such as chicken, mudcrab, etc
  float reachPveHotfixMult =
    (actor.GetBaseId() <= 0x7 && targetActor.GetBaseId() <= 0x7)
    ? 1.f
    : std::numeric_limits<float>::infinity();

  float reach = GetReach(actor, hitData.source, reachPveHotfixMult);

  // For bow/crossbow shots we don't want to check melee radius
  if (IsBowOrCrossbowShot(hitData, actor.GetParent())) {
    constexpr float kExteriorCellWidthUnits = 4096.f;
    reach = kExteriorCellWidthUnits * 2;
  }

  return reach * reach > sqrDistance;
}

bool CanHit(const MpActor& actor, const HitData& hitData,
            const std::chrono::duration<float>& timePassed)
{
  WorldState* espmProvider = actor.GetParent();
  auto weapDNAM =
    espm::GetData<espm::WEAP>(hitData.source, espmProvider).weapDNAM;

  if (weapDNAM) {
    float speedMult = weapDNAM->speed;
    return timePassed.count() >= (1.1 * (1 / speedMult)) -
      (1.1 * (1 / speedMult) * (speedMult <= 0.75 ? 0.45 : 0.3));
  }

  throw std::runtime_error(
    fmt::format("Cannot get weapon speed from source: {0:x}", hitData.source));
}

bool ShouldBeBlocked(const MpActor& aggressor, const MpActor& target)
{
  NiPoint3 targetViewDirection = target.GetViewDirection();
  NiPoint3 aggressorDirection = aggressor.GetPos() - target.GetPos();
  if (targetViewDirection * aggressorDirection <= 0) {
    return false;
  }
  float angle =
    std::acos((targetViewDirection * aggressorDirection) /
              (targetViewDirection.Length() * aggressorDirection.Length()));
  return angle < 1;
}
}

void ActionListener::OnHit(const RawMessageData& rawMsgData, const HitMessage& msg)
{
  MpActor* myActor = partOne.serverState.ActorByUser(rawMsgData.userId);

  if (!myActor) {
    return spdlog::error(
      "ActionListener::OnHit - no Actor attached to userId {}",
      rawMsgData.userId);
  }

  MpActor* aggressor = nullptr;

  HitData hitData = msg.data;
  if (hitData.aggressor == 0x14) {
    aggressor = myActor;
    hitData.aggressor = aggressor->GetFormId();
  } else {
    aggressor = &partOne.worldState.GetFormAt<MpActor>(hitData.aggressor);
    auto it = partOne.worldState.hosters.find(hitData.aggressor);
    if (it == partOne.worldState.hosters.end() ||
        it->second != myActor->GetFormId()) {
      spdlog::error("SendToNeighbours - No permission to send OnHit with "
                    "aggressor actor {:x}",
                    aggressor->GetFormId());
      return;
    }
  }

  if (hitData.target == 0x14) {
    hitData.target = myActor->GetFormId();
  }

  MpForm* targetForm = partOne.worldState.LookupFormById(hitData.target).get();
  MpObjectReference* targetRef =
    targetForm ? targetForm->AsObjectReference() : nullptr;
  if (!targetRef) {
    spdlog::error("ActionListener::OnHit - MpObjectReference not found for "
                  "hitData.target {:x}",
                  hitData.target);
    return;
  }

  const FormDesc& aggressorCellOrWorld = aggressor->GetCellOrWorld();
  const FormDesc& targetCellOrWorld = targetRef->GetCellOrWorld();

  if (aggressorCellOrWorld != targetCellOrWorld) {
    const std::vector<std::string>& files = partOne.worldState.espmFiles;
    spdlog::error(
      "ActionListener::OnHit - aggressor and targetRef are in different cells "
      "or world. Aggressor: {:x}, targetRef: {:x}, cellOrWorld of aggressor: "
      "{:x}, cellOrWorld of targetRef: {:x}",
      aggressor->GetFormId(), targetRef->GetFormId(),
      aggressorCellOrWorld.ToFormId(files), targetCellOrWorld.ToFormId(files));
    return;
  }

  // TODO: repair IsDistanceValid instead
  if (!IsBowOrCrossbowShot(hitData, &partOne.worldState)) {
    const NiPoint3& aggressorPos = aggressor->GetPos();
    const NiPoint3& targetPos = targetRef->GetPos();
    constexpr float kExteriorCellWidthUnits = 4096.f;
    if ((aggressorPos - targetPos).SqrLength() >
        kExteriorCellWidthUnits * kExteriorCellWidthUnits) {
      spdlog::error("ActionListener::OnHit - aggressor and targetRef are too "
                    "distant. Aggressor: {:x}, targetRef: {:x}",
                    aggressor->GetFormId(), targetRef->GetFormId());
      return;
    }
  }

  if (aggressor->IsDead()) {
    spdlog::debug(fmt::format("{:x} actor is dead and can't attack. "
                              "requesting respawn in order to fix death state",
                              aggressor->GetFormId()));
    aggressor->RespawnWithDelay(true);
    return;
  }

  auto sourceInEspm =
    partOne.GetEspm().GetBrowser().LookupById(hitData.source);

  const bool isSourceSpell =
    sourceInEspm.rec && sourceInEspm.rec->GetType() == espm::SPEL::kType;

  const auto equipment = aggressor->GetEquipment();

  if (isSourceSpell && equipment.IsSpellEquipped(hitData.source)) {
    OnSpellHit(aggressor, targetRef, hitData);
    return;
  }

  const bool isUnarmed = IsUnarmedAttack(hitData.source);

  if (equipment.inv.HasItem(hitData.source) || isUnarmed) {
    OnWeaponHit(aggressor, targetRef, hitData, isUnarmed);
    return;
  }

  if (aggressor->GetInventory().HasItem(hitData.source) == false) {
    spdlog::debug("{:x} actor has no {:x} weapon and can't attack",
                  hitData.aggressor, hitData.source);
  }

  spdlog::debug("{:x} weapon is not equipped by {:x} actor and cannot be used",
                hitData.source, hitData.aggressor);
}

void ActionListener::OnUpdateAnimVariables(const RawMessageData& rawMsgData, const UpdateAnimVariablesMessage& msg)
{
  const MpActor* myActor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!myActor) {
    throw std::runtime_error("Unable to change values without Actor attached");
  }
  SendToNeighbours(myActor->idx, rawMsgData);
}

void ActionListener::OnSpellCast(const RawMessageData& rawMsgData, const SpellCastMessage& msg)
{
  MpActor* myActor = partOne.serverState.ActorByUser(rawMsgData.userId);

  if (!myActor) {
    throw std::runtime_error("Unable to change values without Actor attached");
  }

  MpActor* caster = nullptr;

  SpellCastData spellCastData = msg.data;

  if (spellCastData.caster == 0x14 ||
      spellCastData.caster == myActor->GetFormId()) {
    caster = myActor;
    spellCastData.caster = caster->GetFormId();
  } else {
    caster = &partOne.worldState.GetFormAt<MpActor>(spellCastData.caster);
    const auto it = partOne.worldState.hosters.find(spellCastData.caster);

    if (it == partOne.worldState.hosters.end() ||
        it->second != myActor->GetFormId()) {
      spdlog::error(
        "SendToNeighbours - No permission to send OnSpellCast with "
        "caster actor {:x}",
        caster->GetFormId());
      return;
    }
  }

  if (spellCastData.target == 0x14) {
    spellCastData.target = myActor->GetFormId();
  }

  if (caster->IsDead()) {
    spdlog::info(fmt::format("{:x} actor is dead and can't spell cast. "
                             "requesting respawn in order to fix death state",
                             caster->GetFormId()));
    caster->RespawnWithDelay(true);
    return;
  }

  const auto equipment = caster->GetEquipment();

  if (equipment.IsSpellEquipped(spellCastData.spell) == false) {
    spdlog::info("ActionListener::OnSpellCast - spell {0:x} not "
                 "found in equipment",
                 spellCastData.spell);
    return;
  }

  SendToNeighbours(myActor->idx, rawMsgData);

  if (spellCastData.interruptCast) {
    return;
  }

  auto& browser = partOne.worldState.GetEspm().GetBrowser();

  const std::array<VarValue, 1> args{ VarValue(
    std::make_shared<EspmGameObject>(
      browser.LookupById(spellCastData.spell))) };

  caster->SendPapyrusEvent("OnSpellCast", args.data(), args.size());

  const auto targetRef = std::dynamic_pointer_cast<MpObjectReference>(
    partOne.worldState.LookupFormById(spellCastData.target));

  if (!targetRef) {
    spdlog::info(
      "ActionListener::OnSpellCast - MpObjectReference not found for "
      "spellCastData.target {:x}",
      spellCastData.target);
    return;
  }

  // TODO: apply magic effects if this is not a fireball-like spell.
  // Previous attempt was not successful, so it was deleted.
}

void ActionListener::OnUnknown(const RawMessageData& rawMsgData)
{
  spdlog::warn("ActionListener::OnUnknown - Got unhandled message");
}

void ActionListener::OnSpellHit(MpActor* aggressor,
                                MpObjectReference* targetRef,
                                const HitData& hitData)
{
  SendPapyrusOnHitEvent(aggressor, targetRef, hitData);

  auto* targetActorPtr = targetRef ? targetRef->AsActor() : nullptr;
  if (!targetActorPtr) {
    return; // Not an actor, damage calculation is not needed
  }

  auto targetActorValues = targetActorPtr->GetChangeForm().actorValues;

  SpellCastData spellCastData{ aggressor->GetFormId(),
                               targetActorPtr->GetFormId(),
                               hitData.source,
                               false,
                               false,
                               SpellType::Left };

  float damage =
    partOne.CalculateDamage(*aggressor, *targetActorPtr, spellCastData);
  damage = damage <= 0.f ? 0.f : damage;

  targetActorValues.healthPercentage = CalculateCurrentHealthPercentage(
    *targetActorPtr, damage, targetActorValues.healthPercentage, nullptr);

  static const auto kHealthAvFilter =
    std::vector<espm::ActorValue>{ espm::ActorValue::Health };

  targetActorPtr->NetSetPercentages(targetActorValues, aggressor,
                                    kHealthAvFilter);

  spdlog::info("OnSpellHit - Target {0:x} is hit by {1:x} spell on {2} "
               "damage. By caster: {3:x})",
               spellCastData.target, spellCastData.spell, damage,
               spellCastData.caster);
}

void ActionListener::OnWeaponHit(MpActor* aggressor,
                                 MpObjectReference* targetRef, HitData hitData,
                                 [[maybe_unused]] bool isUnarmed)
{
  const auto currentHitTime = std::chrono::steady_clock::now();

  SendPapyrusOnHitEvent(aggressor, targetRef, hitData);

  auto* targetActorPtr = targetRef ? targetRef->AsActor() : nullptr;
  if (!targetActorPtr) {
    return; // Not an actor, damage calculation is not needed
  }

  auto& targetActor = *targetActorPtr;

  const auto lastHitTime = aggressor->GetLastHitTime();
  const std::chrono::duration<float> timePassed = currentHitTime - lastHitTime;

  if (!CanHit(*aggressor, hitData, timePassed)) {
    WorldState* espmProvider = targetActor.GetParent();
    auto weapDNAM =
      espm::GetData<espm::WEAP>(hitData.source, espmProvider).weapDNAM;
    float expectedAttackTime = (1.1 * (1 / weapDNAM->speed)) -
      (1.1 * (1 / weapDNAM->speed) * (weapDNAM->speed <= 0.75 ? 0.45 : 0.3));
    spdlog::debug(
      "OnWeaponHit - Target {0:x} is not available for attack due to fast "
      "attack speed. Weapon: {1:x}. Elapsed time: {2}. Expected attack time: "
      "{3}",
      hitData.target, hitData.source, timePassed.count(), expectedAttackTime);
    return;
  }

  // if (IsDistanceValid(*aggressor, targetActor, hitData) == false) {
  //   float distance =
  //     std::sqrt(GetSqrDistanceToBounds(*aggressor, targetActor));

  //   // TODO: fix bounding boxes for creatures such as chicken, mudcrab, etc
  //   float reachPveHotfixMult =
  //     (aggressor->GetBaseId() <= 0x7 && targetActor.GetBaseId() <= 0x7)
  //     ? 1.f
  //     : std::numeric_limits<float>::infinity();

  //   float reach = GetReach(*aggressor, hitData.source, reachPveHotfixMult);
  //   uint32_t aggressorId = aggressor->GetFormId();
  //   uint32_t targetId = targetActor.GetFormId();
  //   spdlog::debug(
  //     fmt::format("{:x} actor can't reach {:x} target because distance {} is
  //     "
  //                 "greater then first actor attack radius {}",
  //                 aggressorId, targetId, distance, reach));
  //   return;
  // }

  ActorValues currentActorValues = targetActor.GetChangeForm().actorValues;

  float healthPercentage = currentActorValues.healthPercentage;

  if (targetActor.IsBlockActive()) {
    if (ShouldBeBlocked(*aggressor, targetActor)) {
      bool isRemoteBowAttack = false;

      auto sourceLookupResult =
        targetActor.GetParent()->GetEspm().GetBrowser().LookupById(
          hitData.source);
      if (sourceLookupResult.rec &&
          sourceLookupResult.rec->GetType() == espm::WEAP::kType) {
        auto weapData =
          espm::GetData<espm::WEAP>(hitData.source, targetActor.GetParent());
        if (weapData.weapDNAM) {
          if (weapData.weapDNAM->animType == espm::WEAP::AnimType::Bow ||
              weapData.weapDNAM->animType == espm::WEAP::AnimType::Crossbow) {
            if (!hitData.isBashAttack) {
              isRemoteBowAttack = true;
            }
          }
        }
      }

      bool isBlockingByShield = false;

      auto targetActorEquipmentEntries =
        targetActor.GetEquipment().inv.entries;
      for (auto& entry : targetActorEquipmentEntries) {
        if (entry.GetWorn() != Inventory::Worn::None) {
          auto res =
            targetActor.GetParent()->GetEspm().GetBrowser().LookupById(
              entry.baseId);
          if (res.rec && res.rec->GetType() == espm::ARMO::kType) {
            auto data =
              espm::GetData<espm::ARMO>(entry.baseId, targetActor.GetParent());
            bool isShield = data.equipSlotId > 0;
            if (isShield) {
              isBlockingByShield = isShield;
            }
          }
        }
      }

      if (!isRemoteBowAttack || isBlockingByShield) {
        hitData.isHitBlocked = true;
      }
    }
  }

  float damage = partOne.CalculateDamage(*aggressor, targetActor, hitData);
  damage = damage < 0.f ? 0.f : damage;
  float outBaseHealth = 0.f;
  currentActorValues.healthPercentage = CalculateCurrentHealthPercentage(
    targetActor, damage, healthPercentage, &outBaseHealth);

  currentActorValues.healthPercentage =
    currentActorValues.healthPercentage < 0.f
    ? 0.f
    : currentActorValues.healthPercentage;

  targetActor.NetSetPercentages(
    currentActorValues, aggressor,
    std::vector<espm::ActorValue>{ espm::ActorValue::Health });
  aggressor->SetLastHitTime();

  spdlog::debug(
    "OnWeaponHit - Target {0:x} is hit by {1} damage. Percentage was: {3}, "
    "percentage now: {2}, base health: {4})",
    hitData.target, damage, currentActorValues.healthPercentage,
    healthPercentage, outBaseHealth);
}

void ActionListener::SendPapyrusOnHitEvent(MpActor* aggressor,
                                           MpObjectReference* target,
                                           const HitData& hitData)
{
  auto& browser = partOne.worldState.GetEspm().GetBrowser();
  std::array<VarValue, 7> args;
  args[0] = VarValue(aggressor->ToGameObject()); // akAgressor
  args[1] = VarValue(std::make_shared<EspmGameObject>(
    browser.LookupById(hitData.source)));    // akSource
  args[2] = VarValue::None();                // akProjectile
  args[3] = VarValue(hitData.isPowerAttack); // abPowerAttack
  args[4] = VarValue(hitData.isSneakAttack); // abSneakAttack
  args[5] = VarValue(hitData.isBashAttack);  // abBashAttack
  args[6] = VarValue(hitData.isHitBlocked);  // abHitBlocked
  target->SendPapyrusEvent("OnHit", args.data(), args.size());
}
