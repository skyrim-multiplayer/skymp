#include "ActionListener.h"
#include "AnimationSystem.h"
#include "ConsoleCommands.h"
#include "CropRegeneration.h"
#include "DummyMessageOutput.h"
#include "Exceptions.h"
#include "FindRecipe.h"
#include "GetBaseActorValues.h"
#include "HitData.h"
#include "MathUtils.h"
#include "MovementValidation.h"
#include "MpObjectReference.h"
#include "MsgType.h"
#include "UserMessageOutput.h"
#include "WorldState.h"
#include "papyrus-vm/Utils.h"
#include "script_objects/EspmGameObject.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <unordered_set>

#include "UpdateEquipmentMessage.h"

MpActor* ActionListener::SendToNeighbours(
  uint32_t idx, const simdjson::dom::element& jMessage,
  Networking::UserId userId, Networking::PacketData data, size_t length,
  bool reliable)
{
  MpActor* myActor = partOne.serverState.ActorByUser(userId);
  // The old behavior is doing nothing in that case. This is covered by tests
  if (!myActor) {
    spdlog::warn("SendToNeighbours - No actor assigned to user");
    return nullptr;
  }

  MpActor* actor =
    dynamic_cast<MpActor*>(partOne.worldState.LookupFormByIdx(idx));
  if (!actor) {
    spdlog::error("SendToNeighbours - Target actor doesn't exist");
    return nullptr;
  }

  if (idx != myActor->GetIdx()) {
    auto it = partOne.worldState.hosters.find(actor->GetFormId());
    if (it == partOne.worldState.hosters.end() ||
        it->second != myActor->GetFormId()) {
      if (idx == 0) {
        spdlog::warn("SendToNeighbours - idx=0, <Message>::ReadJson or "
                     "similar is probably incorrect");
      }
      spdlog::error("SendToNeighbours - No permission to update actor {:x}",
                    actor->GetFormId());
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
  return SendToNeighbours(idx, rawMsgData.parsed, rawMsgData.userId,
                          rawMsgData.unparsed, rawMsgData.unparsedLength,
                          reliable);
}

void ActionListener::OnCustomPacket(const RawMessageData& rawMsgData,
                                    simdjson::dom::element& content)
{
  for (auto& listener : partOne.GetListeners())
    listener->OnCustomPacket(rawMsgData.userId, content);
}

void ActionListener::OnUpdateMovement(const RawMessageData& rawMsgData,
                                      uint32_t idx, const NiPoint3& pos,
                                      const NiPoint3& rot, bool isInJumpState,
                                      bool isWeapDrawn, bool isBlocking,
                                      uint32_t worldOrCell)
{
  auto actor = SendToNeighbours(idx, rawMsgData);
  if (actor) {
    DummyMessageOutput msgOutputDummy;
    UserMessageOutput msgOutput(partOne.GetSendTarget(), rawMsgData.userId);

    bool isMe = partOne.serverState.ActorByUser(rawMsgData.userId) == actor;

    bool teleportFlag = actor->GetTeleportFlag();
    actor->SetTeleportFlag(false);

    static const NiPoint3 reallyWrongPos = {
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity()
    };

    auto& espmFiles = actor->GetParent()->espmFiles;
    if (!MovementValidation::Validate(
          *actor, teleportFlag ? reallyWrongPos : pos,
          FormDesc::FromFormId(worldOrCell, espmFiles),
          isMe ? static_cast<IMessageOutput&>(msgOutput)
               : static_cast<IMessageOutput&>(msgOutputDummy),
          espmFiles)) {
      return;
    }

    if (!isBlocking) {
      actor->IncreaseBlockCount();
    } else {
      actor->ResetBlockCount();
    }

    actor->SetPos(pos);
    actor->SetAngle(rot);
    actor->SetAnimationVariableBool("bInJumpState", isInJumpState);
    actor->SetAnimationVariableBool("_skymp_isWeapDrawn", isWeapDrawn);
    actor->SetAnimationVariableBool("IsBlocking", isBlocking);
    if (actor->GetBlockCount() == 5) {
      actor->SetIsBlockActive(false);
      actor->ResetBlockCount();
    }

    if (partOne.worldState.lastMovUpdateByIdx.size() <= idx) {
      auto newSize = static_cast<size_t>(idx) + 1;
      partOne.worldState.lastMovUpdateByIdx.resize(newSize);
    }
    partOne.worldState.lastMovUpdateByIdx[idx] =
      std::chrono::system_clock::now();
  }
}

void ActionListener::OnUpdateAnimation(const RawMessageData& rawMsgData,
                                       uint32_t idx,
                                       const AnimationData& animationData)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor) {
    return;
  }

  WorldState* espmProvider = actor->GetParent();
  if (!espmProvider) {
    return;
  }

  partOne.animationSystem.Process(actor, animationData);

  SendToNeighbours(idx, rawMsgData);
}

void ActionListener::OnUpdateAppearance(const RawMessageData& rawMsgData,
                                        uint32_t idx,
                                        const Appearance& appearance)
{ // TODO: validate

  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor || !actor->IsRaceMenuOpen())
    return;

  actor->SetRaceMenuOpen(false);
  actor->SetAppearance(&appearance);
  SendToNeighbours(idx, rawMsgData, true);
}

void ActionListener::OnUpdateEquipment(
  const RawMessageData& rawMsgData, const uint32_t idx,
  const simdjson::dom::element& data, const Inventory& equipmentInv,
  const uint32_t leftSpell, const uint32_t rightSpell,
  const uint32_t voiceSpell, const uint32_t instantSpell)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);

  if (!actor) {
    return;
  }

  if (leftSpell > 0 && !actor->IsSpellLearned(leftSpell)) {
    spdlog::debug(
      "OnUpdateEquipment result false. Spell with id ({}) not learned",
      leftSpell);
    return;
  }

  if (rightSpell > 0 && !actor->IsSpellLearned(rightSpell)) {
    spdlog::debug(
      "OnUpdateEquipment result false. Spell with id ({}) not learned",
      leftSpell);
    return;
  }

  if (voiceSpell > 0 && !actor->IsSpellLearned(voiceSpell)) {
    spdlog::debug(
      "OnUpdateEquipment result false. Spell with id ({}) not learned",
      leftSpell);
    return;
  }

  if (instantSpell > 0 && !actor->IsSpellLearned(instantSpell)) {
    spdlog::debug(
      "OnUpdateEquipment result false. Spell with id ({}) not learned",
      leftSpell);
    return;
  }

  const auto& inventory = actor->GetInventory();

  for (auto& [baseId, count, _] : equipmentInv.entries) {
    if (!inventory.HasItem(baseId)) {
      spdlog::debug(
        "OnUpdateEquipment result false. The inventory does not contain item "
        "with id {:x}",
        baseId);
      return;
    }
  }

  SendToNeighbours(idx, rawMsgData, true);
  actor->SetEquipment(simdjson::minify(data));
}

void ActionListener::OnActivate(const RawMessageData& rawMsgData,
                                uint32_t caster, uint32_t target)
{
  if (!partOne.HasEspm())
    throw std::runtime_error("No loaded esm or esp files are found");

  const auto ac = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!ac)
    throw std::runtime_error("Can't do this without Actor attached");

  auto it = partOne.worldState.hosters.find(caster);
  auto hosterId = it == partOne.worldState.hosters.end() ? 0 : it->second;

  if (caster != 0x14) {
    if (hosterId != ac->GetFormId()) {
      std::stringstream ss;
      ss << std::hex << "Bad hoster is attached to caster 0x" << caster
         << ", expected 0x" << ac->GetFormId() << ", but found 0x" << hosterId;
      throw std::runtime_error(ss.str());
    }
  }

  auto targetPtr = std::dynamic_pointer_cast<MpObjectReference>(
    partOne.worldState.LookupFormById(target));
  if (!targetPtr)
    return;

  targetPtr->Activate(
    caster == 0x14 ? *ac
                   : partOne.worldState.GetFormAt<MpObjectReference>(caster));
  if (hosterId) {
    auto actor = std::dynamic_pointer_cast<MpActor>(
      partOne.worldState.LookupFormById(caster));
    if (actor) {
      actor->EquipBestWeapon();
    }
  }
}

void ActionListener::OnPutItem(const RawMessageData& rawMsgData,
                               uint32_t target, const Inventory::Entry& entry)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(target);

  if (!actor)
    return; // TODO: Throw error instead
  ref.PutItem(*actor, entry);
}

void ActionListener::OnTakeItem(const RawMessageData& rawMsgData,
                                uint32_t target, const Inventory::Entry& entry)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(target);

  if (!actor)
    return; // TODO: Throw error instead
  ref.TakeItem(*actor, entry);
}

void ActionListener::OnDropItem(const RawMessageData& rawMsgData,
                                uint32_t baseId, const Inventory::Entry& entry)
{
  MpActor* ac = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!ac) {
    throw std::runtime_error(fmt::format(
      "Unable to drop an item from user with id: {:x}.", rawMsgData.userId));
  }
  ac->DropItem(baseId, entry);
}

namespace {

VarValue VarValueFromJson(const simdjson::dom::element& parentMsg,
                          const simdjson::dom::element& element)
{
  static const auto kKey = JsonPointer("returnValue");

  // TODO: DOUBLE, STRING ...
  switch (element.type()) {
    case simdjson::dom::element_type::INT64:
    case simdjson::dom::element_type::UINT64: {
      int32_t v;
      ReadEx(parentMsg, kKey, &v);
      return VarValue(v);
    }
    case simdjson::dom::element_type::BOOL: {
      bool v;
      ReadEx(parentMsg, kKey, &v);
      return VarValue(v);
    }
    case simdjson::dom::element_type::NULL_VALUE:
      return VarValue::None();
    default:
      break;
  }
  throw std::runtime_error("VarValueFromJson - Unsupported json type " +
                           std::to_string(static_cast<int>(element.type())));
}

}
void ActionListener::OnFinishSpSnippet(const RawMessageData& rawMsgData,
                                       uint32_t snippetIdx,
                                       simdjson::dom::element& returnValue)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor)
    throw std::runtime_error(
      "Unable to finish SpSnippet: No Actor found for user " +
      std::to_string(rawMsgData.userId));

  actor->ResolveSnippet(snippetIdx,
                        VarValueFromJson(rawMsgData.parsed, returnValue));
}

void ActionListener::OnEquip(const RawMessageData& rawMsgData, uint32_t baseId)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor) {
    throw std::runtime_error(
      "Unable to finish SpSnippet: No Actor found for user " +
      std::to_string(rawMsgData.userId));
  }

  std::ignore = actor->OnEquip(baseId);
}

void ActionListener::OnConsoleCommand(
  const RawMessageData& rawMsgData, const std::string& consoleCommandName,
  const std::vector<ConsoleCommands::Argument>& args)
{
  MpActor* me = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (me)
    ConsoleCommands::Execute(*me, consoleCommandName, args);
}

void UseCraftRecipe(MpActor* me, const espm::COBJ* recipeUsed,
                    espm::CompressedFieldsCache& cache,
                    const espm::CombineBrowser& br, int espmIdx)
{
  auto recipeData = recipeUsed->GetData(cache);
  auto mapping = br.GetCombMapping(espmIdx);

  std::vector<Inventory::Entry> entries;
  for (auto& entry : recipeData.inputObjects) {
    auto formId = espm::utils::GetMappedId(entry.formId, *mapping);
    entries.push_back({ formId, entry.count });
  }

  auto outputFormId =
    espm::utils::GetMappedId(recipeData.outputObjectFormId, *mapping);

  if (spdlog::should_log(spdlog::level::debug)) {
    std::string s = fmt::format("User formId={:#x} crafted", me->GetFormId());
    for (const auto& entry : entries) {
      s += fmt::format(" -{:#x} x{}", entry.baseId, entry.count);
    }
    s += fmt::format(" +{:#x} x{}", outputFormId, recipeData.outputCount);
    spdlog::debug("{}", s);
  }

  auto recipeId = espm::utils::GetMappedId(recipeUsed->GetId(), *mapping);

  if (me->MpApiCraft(outputFormId, recipeData.outputCount, recipeId)) {
    spdlog::trace("onCraft - not blocked by gamemode");
    me->RemoveItems(entries);
    me->AddItem(outputFormId, recipeData.outputCount);
  } else {
    spdlog::trace("onCraft - blocked by gamemode");
  }
}

void ActionListener::OnCraftItem(const RawMessageData& rawMsgData,
                                 const Inventory& inputObjects,
                                 uint32_t workbenchId, uint32_t resultObjectId)
{
  auto& workbench =
    partOne.worldState.GetFormAt<MpObjectReference>(workbenchId);

  auto& br = partOne.worldState.GetEspm().GetBrowser();
  auto& cache = partOne.worldState.GetEspmCache();
  auto base = br.LookupById(workbench.GetBaseId());

  spdlog::debug("User {} tries to craft {:#x} on workbench {:#x}",
                rawMsgData.userId, resultObjectId, workbenchId);

  bool isFurnitureOrActivator =
    base.rec->GetType() == "FURN" || base.rec->GetType() == "ACTI";
  if (!isFurnitureOrActivator) {
    throw std::runtime_error("Unable to use " +
                             base.rec->GetType().ToString() + " as workbench");
  }

  int espmIdx = 0;
  auto recipeUsed = FindRecipe(br, inputObjects, resultObjectId, &espmIdx);

  if (!recipeUsed) {
    throw std::runtime_error(
      fmt::format("Recipe not found: inputObjects={}, workbenchId={:#x}, "
                  "resultObjectId={:#x}",
                  inputObjects.ToJson().dump(), workbenchId, resultObjectId));
  }

  MpActor* me = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!me) {
    throw std::runtime_error("Unable to craft without Actor attached");
  }

  UseCraftRecipe(me, recipeUsed, cache, br, espmIdx);
}

void ActionListener::OnHostAttempt(const RawMessageData& rawMsgData,
                                   uint32_t remoteId)
{
  MpActor* me = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!me) {
    throw std::runtime_error("Unable to host without actor attached");
  }

  auto& remote = partOne.worldState.GetFormAt<MpObjectReference>(remoteId);

  auto user = partOne.serverState.UserByActor(dynamic_cast<MpActor*>(&remote));
  if (user != Networking::InvalidUserId) {
    return;
  }

  auto& hoster = partOne.worldState.hosters[remoteId];
  const uint32_t prevHoster = hoster;

  auto remoteIdx = remote.GetIdx();

  std::optional<std::chrono::system_clock::time_point> lastRemoteUpdate;
  if (partOne.worldState.lastMovUpdateByIdx.size() > remoteIdx) {
    lastRemoteUpdate = partOne.worldState.lastMovUpdateByIdx[remoteIdx];
  }

  const auto hostResetTimeout = std::chrono::seconds(2);

  if (hoster == 0 || !lastRemoteUpdate ||
      std::chrono::system_clock::now() - *lastRemoteUpdate >
        hostResetTimeout) {
    partOne.GetLogger().info("Hoster changed from {0:x} to {0:x}", prevHoster,
                             me->GetFormId());
    hoster = me->GetFormId();
    remote.UpdateHoster(hoster);

    auto remoteAsActor = dynamic_cast<MpActor*>(&remote);

    if (remoteAsActor) {
      remoteAsActor->EquipBestWeapon();
    }

    uint64_t longFormId = remote.GetFormId();
    if (remoteAsActor && longFormId < 0xff000000) {
      longFormId += 0x100000000;
    }

    Networking::SendFormatted(&partOne.GetSendTarget(), rawMsgData.userId,
                              R"({ "type": "hostStart", "target": %llu })",
                              longFormId);

    // Otherwise, health percentage would remain unsynced until someone hits
    // npc
    auto formId = remote.GetFormId();
    partOne.worldState.SetTimer(std::chrono::seconds(1))
      .Then([this, formId](Viet::Void) {
        // Check if form is still here
        auto& remote = partOne.worldState.GetFormAt<MpActor>(formId);

        auto changeForm = remote.GetChangeForm();

        ChangeValuesMessage msg;
        msg.idx = remote.GetIdx();
        msg.data.health = changeForm.actorValues.healthPercentage;
        msg.data.magicka = changeForm.actorValues.magickaPercentage;
        msg.data.stamina = changeForm.actorValues.staminaPercentage;
        remote.SendToUser(msg, true); // in fact sends to hoster
      });

    if (MpActor* prevHosterActor = dynamic_cast<MpActor*>(
          partOne.worldState.LookupFormById(prevHoster).get())) {
      auto prevHosterUser = partOne.serverState.UserByActor(prevHosterActor);
      if (prevHosterUser != Networking::InvalidUserId &&
          prevHosterUser != rawMsgData.userId) {
        Networking::SendFormatted(&partOne.GetSendTarget(), prevHosterUser,
                                  R"({ "type": "hostStop", "target": %llu })",
                                  longFormId);
      }
    }
  }
}

void ActionListener::OnCustomEvent(const RawMessageData& rawMsgData,
                                   const char* eventName,
                                   simdjson::dom::element& args)
{
  auto ac = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!ac) {
    return;
  }

  if (eventName[0] != '_') {
    return;
  }

  for (auto& listener : partOne.GetListeners()) {
    listener->OnMpApiEvent(eventName, args, ac->GetFormId());
  }
}

void ActionListener::OnChangeValues(const RawMessageData& rawMsgData,
                                    const ActorValues& newActorValues)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor) {
    throw std::runtime_error("Unable to change values without Actor attached");
  }
  auto now = std::chrono::steady_clock::now();

  float timeAfterRegeneration = CropPeriodAfterLastRegen(
    actor->GetDurationOfAttributesPercentagesUpdate(now).count());

  ActorValues currentActorValues = actor->GetChangeForm().actorValues;
  float health = newActorValues.healthPercentage;
  float magicka = newActorValues.magickaPercentage;
  float stamina = newActorValues.staminaPercentage;

  if (newActorValues.healthPercentage != currentActorValues.healthPercentage) {
    currentActorValues.healthPercentage =
      CropHealthRegeneration(health, timeAfterRegeneration, actor);
  }
  if (newActorValues.magickaPercentage !=
      currentActorValues.magickaPercentage) {
    currentActorValues.magickaPercentage =
      CropMagickaRegeneration(magicka, timeAfterRegeneration, actor);
  }
  if (newActorValues.staminaPercentage !=
      currentActorValues.staminaPercentage) {
    currentActorValues.staminaPercentage =
      CropStaminaRegeneration(stamina, timeAfterRegeneration, actor);
  }

  if (!MathUtils::IsNearlyEqual(currentActorValues.healthPercentage,
                                newActorValues.healthPercentage) ||
      !MathUtils::IsNearlyEqual(currentActorValues.magickaPercentage,
                                newActorValues.magickaPercentage) ||
      !MathUtils::IsNearlyEqual(currentActorValues.staminaPercentage,
                                newActorValues.staminaPercentage)) {
    actor->NetSendChangeValues(currentActorValues);
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
  uint32_t baseId = actor.GetBaseId();
  uint32_t raceId = actor.GetRaceId();
  WorldState* espmProvider = actor.GetParent();
  float baseHealth =
    GetBaseActorValues(espmProvider, baseId, raceId, actor.GetTemplateChain())
      .health;

  if (outBaseHealth) {
    *outBaseHealth = baseHealth;
  }

  float damagePercentage = damage / baseHealth;
  float currentHealthPercentage = healthPercentage - damagePercentage;
  return currentHealthPercentage;
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
  if (!hitData.isBashAttack) {
    constexpr float kExteriorCellWidthUnits = 4096.f;
    if (auto worldState = actor.GetParent()) {
      if (worldState->HasEspm()) {
        auto weapDNAM =
          espm::GetData<espm::WEAP>(hitData.source, worldState).weapDNAM;
        if (weapDNAM->animType == espm::WEAP::AnimType::Bow) {
          reach = kExteriorCellWidthUnits;
        } else if (weapDNAM->animType == espm::WEAP::AnimType::Crossbow) {
          reach = kExteriorCellWidthUnits;
        }
      }
    }
  }

  return reach * reach > sqrDistance;
}

bool IsAvailableForNextAttack(const MpActor& actor, const HitData& hitData,
                              const std::chrono::duration<float>& timePassed)
{
  WorldState* espmProvider = actor.GetParent();
  auto weapDNAM =
    espm::GetData<espm::WEAP>(hitData.source, espmProvider).weapDNAM;
  if (weapDNAM) {
    float speedMult = weapDNAM->speed;
    return timePassed.count() >= (1.1 * (1 / speedMult)) -
      (1.1 * (1 / speedMult) * (speedMult <= 0.75 ? 0.45 : 0.3));
  } else {
    throw std::runtime_error(fmt::format(
      "Cannot get weapon speed from source: {0:x}", hitData.source));
  }
}

bool ShouldBeBlocked(const MpActor& aggressor, const MpActor& target)
{
  NiPoint3 targetViewDirection = target.GetViewDirection();
  NiPoint3 aggressorViewDirection = aggressor.GetViewDirection();
  if (targetViewDirection * aggressorViewDirection >= 0) {
    return false;
  }
  NiPoint3 aggressorDirection = aggressor.GetPos() - target.GetPos();
  float angle =
    std::acos((targetViewDirection * aggressorDirection) /
              (targetViewDirection.Length() * aggressorDirection.Length()));
  return angle < 1;
}
}

void ActionListener::OnHit(const RawMessageData& rawMsgData_,
                           const HitData& hitData_)
{
  auto currentHitTime = std::chrono::steady_clock::now();
  MpActor* myActor = partOne.serverState.ActorByUser(rawMsgData_.userId);
  if (!myActor) {
    throw std::runtime_error("Unable to change values without Actor attached");
  }

  MpActor* aggressor = nullptr;

  HitData hitData = hitData_;
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

  if (aggressor->IsDead()) {
    spdlog::debug(fmt::format("{:x} actor is dead and can't attack. "
                              "requesting respawn in order to fix death state",
                              aggressor->GetFormId()));
    aggressor->RespawnWithDelay(true);
    return;
  }

  if (aggressor->GetEquipment().inv.HasItem(hitData.source) == false &&
      IsUnarmedAttack(hitData.source) == false) {

    if (aggressor->GetInventory().HasItem(hitData.source) == false) {
      spdlog::debug("{:x} actor has no {:x} weapon and can't attack",
                    hitData.aggressor, hitData.source);
    }
    spdlog::debug(
      "{:x} weapon is not equipped by {:x} actor and cannot be used",
      hitData.source, hitData.aggressor);
    return;
  };

  auto refr = std::dynamic_pointer_cast<MpObjectReference>(
    partOne.worldState.LookupFormById(hitData.target));
  if (!refr) {
    spdlog::error("ActionListener::OnHit - MpObjectReference not found for "
                  "hitData.target {:x}",
                  hitData.target);
    return;
  }

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
  refr->SendPapyrusEvent("OnHit", args.data(), args.size());

  auto targetActorPtr = dynamic_cast<MpActor*>(refr.get());
  if (!targetActorPtr) {
    return; // Not an actor, damage calculation is not needed
  }

  auto& targetActor = *targetActorPtr;

  auto lastHitTime = targetActor.GetLastHitTime();
  std::chrono::duration<float> timePassed = currentHitTime - lastHitTime;

  if (!IsAvailableForNextAttack(targetActor, hitData, timePassed)) {
    WorldState* espmProvider = targetActor.GetParent();
    auto weapDNAM =
      espm::GetData<espm::WEAP>(hitData.source, espmProvider).weapDNAM;
    float expectedAttackTime = (1.1 * (1 / weapDNAM->speed)) -
      (1.1 * (1 / weapDNAM->speed) * (weapDNAM->speed <= 0.75 ? 0.45 : 0.3));
    spdlog::debug(
      "Target {0:x} is not available for attack due to fast "
      "attack speed. Weapon: {1:x}. Elapsed time: {2}. Expected attack time: "
      "{3}",
      hitData.target, hitData.source, timePassed.count(), expectedAttackTime);
    return;
  }

  if (IsDistanceValid(*aggressor, targetActor, hitData) == false) {
    float distance =
      std::sqrt(GetSqrDistanceToBounds(*aggressor, targetActor));

    // TODO: fix bounding boxes for creatures such as chicken, mudcrab, etc
    float reachPveHotfixMult =
      (aggressor->GetBaseId() <= 0x7 && targetActor.GetBaseId() <= 0x7)
      ? 1.f
      : std::numeric_limits<float>::infinity();

    float reach = GetReach(*aggressor, hitData.source, reachPveHotfixMult);
    uint32_t aggressorId = aggressor->GetFormId();
    uint32_t targetId = targetActor.GetFormId();
    spdlog::debug(
      fmt::format("{:x} actor can't reach {:x} target because distance {} is "
                  "greater then first actor attack radius {}",
                  aggressorId, targetId, distance, reach));
    return;
  }

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
        if (entry.extra.worn != Inventory::Worn::None) {
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

  targetActor.NetSetPercentages(currentActorValues, aggressor);
  targetActor.SetLastHitTime();

  spdlog::debug("Target {0:x} is hitted by {1} damage. Percentage was: {3}, "
                "percentage now: {2}, base health: {4})",
                hitData.target, damage, currentActorValues.healthPercentage,
                healthPercentage, outBaseHealth);
}

void ActionListener::OnUnknown(const RawMessageData& rawMsgData)
{
  spdlog::error("Got unhandled message: {}",
                simdjson::minify(rawMsgData.parsed));
}
