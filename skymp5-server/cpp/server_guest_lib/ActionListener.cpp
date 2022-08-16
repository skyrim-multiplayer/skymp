#include "ActionListener.h"
#include "ConsoleCommands.h"
#include "CropRegeneration.h"
#include "DummyMessageOutput.h"
#include "EspmGameObject.h"
#include "Exceptions.h"
#include "FindRecipe.h"
#include "GetBaseActorValues.h"
#include "HitData.h"
#include "MovementValidation.h"
#include "MpObjectReference.h"
#include "MsgType.h"
#include "UserMessageOutput.h"
#include "Utils.h"
#include "WorldState.h"
#include <fmt/format.h>
#include <unordered_set>

MpActor* ActionListener::SendToNeighbours(
  uint32_t idx, const simdjson::dom::element& jMessage,
  Networking::UserId userId, Networking::PacketData data, size_t length,
  bool reliable)
{
  MpActor* myActor = partOne.serverState.ActorByUser(userId);
  // The old behavior is doing nothing in that case. This is covered by tests
  if (!myActor)
    return nullptr;

  MpActor* actor =
    dynamic_cast<MpActor*>(partOne.worldState.LookupFormByIdx(idx));
  if (!actor)
    throw std::runtime_error("SendToNeighbours - target actor doesn't exist");

  auto hosterIterator = partOne.worldState.hosters.find(actor->GetFormId());
  uint32_t hosterId = hosterIterator != partOne.worldState.hosters.end()
    ? hosterIterator->second
    : 0;

  if (idx != actor->GetIdx() && hosterId != myActor->GetFormId()) {
    std::stringstream ss;
    ss << std::hex << "You aren't able to update actor with idx " << idx
       << " (your actor's idx is " << actor->GetIdx() << ')';
    throw PublicError(ss.str());
  }

  for (auto listener : actor->GetListeners()) {
    auto listenerAsActor = dynamic_cast<MpActor*>(listener);
    if (listenerAsActor) {
      auto targetuserId = partOne.serverState.UserByActor(listenerAsActor);
      if (targetuserId != Networking::InvalidUserId) {
        partOne.GetSendTarget().Send(targetuserId, data, length, reliable);
      }
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
                                      bool isWeapDrawn, uint32_t worldOrCell)
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

    actor->SetPos(pos);
    actor->SetAngle(rot);
    actor->SetAnimationVariableBool("bInJumpState", isInJumpState);
    actor->SetAnimationVariableBool("_skymp_isWeapDrawn", isWeapDrawn);

    if (partOne.worldState.lastMovUpdateByIdx.size() <= idx) {
      auto newSize = static_cast<size_t>(idx) + 1;
      partOne.worldState.lastMovUpdateByIdx.resize(newSize);
    }
    partOne.worldState.lastMovUpdateByIdx[idx] =
      std::chrono::system_clock::now();
  }
}

void ActionListener::OnUpdateAnimation(const RawMessageData& rawMsgData,
                                       uint32_t idx)
{
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

void ActionListener::OnUpdateEquipment(const RawMessageData& rawMsgData,
                                       uint32_t idx,
                                       simdjson::dom::element& data,
                                       const Inventory& equipmentInv)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor)
    return;

  bool badEq = false;
  for (auto& e : equipmentInv.entries) {
    if (!actor->GetInventory().HasItem(e.baseId)) {
      badEq = true;
      break;
    }
  }

  if (!badEq) {
    SendToNeighbours(idx, rawMsgData, true);
    actor->SetEquipment(simdjson::minify(data));
  }
}

void RecalculateWorn(MpObjectReference& refr)
{
  if (!refr.GetParent()->HasEspm()) {
    return;
  }
  auto& loader = refr.GetParent()->GetEspm();
  auto& cache = refr.GetParent()->GetEspmCache();

  auto ac = dynamic_cast<MpActor*>(&refr);
  if (!ac) {
    return;
  }

  const Equipment eq = ac->GetEquipment();

  Equipment newEq;
  newEq.numChanges = eq.numChanges + 1;
  for (auto& entry : eq.inv.entries) {
    bool isEquipped = entry.extra.worn != Inventory::Worn::None;
    bool isWeap =
      espm::GetRecordType(entry.baseId, refr.GetParent()) == espm::WEAP::kType;
    if (isEquipped && isWeap) {
      continue;
    }
    newEq.inv.AddItems({ entry });
  }

  const Inventory inv = ac->GetInventory();
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
    bestEntry.extra.worn = Inventory::Worn::Right;
    newEq.inv.AddItems({ bestEntry });
  }

  ac->SetEquipment(newEq.ToJson().dump());
  for (auto listener : ac->GetListeners()) {
    auto actor = dynamic_cast<MpActor*>(listener);
    if (!actor) {
      continue;
    }
    std::string s;
    s += Networking::MinPacketId;
    s += nlohmann::json{
      { "t", MsgType::UpdateEquipment },
      { "idx", ac->GetIdx() },
      { "data", newEq.ToJson() }
    }.dump();
    actor->SendToUser(s.data(), s.size(), true);
  }
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
    RecalculateWorn(partOne.worldState.GetFormAt<MpObjectReference>(caster));
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
  static const auto key = JsonPointer("returnValue");

  // TODO: DOUBLE, STRING ...
  switch (element.type()) {
    case simdjson::dom::element_type::INT64:
    case simdjson::dom::element_type::UINT64: {
      int32_t v;
      ReadEx(parentMsg, key, &v);
      return VarValue(v);
    }
    case simdjson::dom::element_type::BOOL: {
      bool v;
      ReadEx(parentMsg, key, &v);
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

bool IsNearlyEqual(float value, float target, float margin = 1.0f / 1024.0f)
{
  return std::abs(target - value) < margin;
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
  if (!actor)
    throw std::runtime_error(
      "Unable to finish SpSnippet: No Actor found for user " +
      std::to_string(rawMsgData.userId));

  actor->OnEquip(baseId);
}

void ActionListener::OnConsoleCommand(
  const RawMessageData& rawMsgData, const std::string& consoleCommandName,
  const std::vector<ConsoleCommands::Argument>& args)
{
  MpActor* me = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (me)
    ConsoleCommands::Execute(*me, consoleCommandName, args);
}

void UseCraftRecipe(MpActor* me, espm::COBJ::Data recipeData,
                    const espm::CombineBrowser& br, int espmIdx)
{
  auto mapping = br.GetCombMapping(espmIdx);
  std::vector<Inventory::Entry> entries;
  for (auto& entry : recipeData.inputObjects) {
    auto formId = espm::GetMappedId(entry.formId, *mapping);
    entries.push_back({ formId, entry.count });
  }
  auto outputFormId =
    espm::GetMappedId(recipeData.outputObjectFormId, *mapping);
  if (spdlog::should_log(spdlog::level::debug)) {
    std::string s = fmt::format("User formId={:#x} crafted", me->GetFormId());
    for (const auto& entry : entries) {
      s += fmt::format(" -{:#x} x{}", entry.baseId, entry.count);
    }
    s += fmt::format(" +{:#x} x{}", outputFormId, recipeData.outputCount);
    spdlog::debug("{}", s);
  }
  me->RemoveItems(entries);
  me->AddItem(espm::GetMappedId(recipeData.outputObjectFormId, *mapping),
              recipeData.outputCount);
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

  if (base.rec->GetType() != "FURN") {
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

  auto recipeData = recipeUsed->GetData(cache);
  UseCraftRecipe(me, recipeData, br, espmIdx);
}

void ActionListener::OnHostAttempt(const RawMessageData& rawMsgData,
                                   uint32_t remoteId)
{
  MpActor* me = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!me)
    throw std::runtime_error("Unable to host without actor attached");

  auto& remote = partOne.worldState.GetFormAt<MpObjectReference>(remoteId);

  auto user = partOne.serverState.UserByActor(dynamic_cast<MpActor*>(&remote));
  if (user != Networking::InvalidUserId)
    return;

  auto& hoster = partOne.worldState.hosters[remoteId];
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
    RecalculateWorn(remote);

    uint64_t longFormId = remote.GetFormId();
    if (dynamic_cast<MpActor*>(&remote) && longFormId < 0xff000000) {
      longFormId += 0x100000000;
    }
    Networking::SendFormatted(&partOne.GetSendTarget(), rawMsgData.userId, R"({ "type": "hostStart", "target": %llu })", longFormId);

    if (MpActor* prevHosterActor = dynamic_cast<MpActor*>(
          partOne.worldState.LookupFormById(prevHoster).get())) {
      auto prevHosterUser = partOne.serverState.UserByActor(prevHosterActor);
      if (prevHosterUser != Networking::InvalidUserId &&
          prevHosterUser != rawMsgData.userId) {
        Networking::SendFormatted(&partOne.GetSendTarget(), prevHosterUser, R"({ "type": "hostStop", "target": %llu })", longFormId);
      }
    }
  }
}

void ActionListener::OnCustomEvent(const RawMessageData& rawMsgData,
                                   const char* eventName,
                                   simdjson::dom::element& args)
{
  auto ac = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!ac)
    return;

  if (eventName[0] != '_')
    return;

  for (auto& listener : partOne.GetListeners()) {
    listener->OnMpApiEvent(eventName, args, ac->GetFormId());
  }
}

void ActionListener::OnChangeValues(const RawMessageData& rawMsgData,
                                    const float healthPercentage,
                                    const float magickaPercentage,
                                    const float staminaPercentage)
{
  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor) {
    throw std::runtime_error("Unable to change values without Actor attached");
  }
  auto now = std::chrono::steady_clock::now();

  float timeAfterRegeneration = CropPeriodAfterLastRegen(actor->GetDurationOfAttributesPercentagesUpdate(now).count());
  
  MpChangeForm changeForm = actor->GetChangeForm();
  float health = healthPercentage;
  float magicka = magickaPercentage;
  float stamina = staminaPercentage;

  if (healthPercentage != changeForm.healthPercentage) {
    health = CropHealthRegeneration(health, timeAfterRegeneration, actor);
  }
  if (magickaPercentage != changeForm.magickaPercentage) {
    magicka = CropMagickaRegeneration(magicka, timeAfterRegeneration, actor);
  }
  if (staminaPercentage != changeForm.staminaPercentage) {
    stamina = CropStaminaRegeneration(stamina, timeAfterRegeneration, actor);
  }

  if (timeAfterRegeneration <= 0.0f && (health != healthPercentage || magicka != magickaPercentage || stamina != staminaPercentage)) {
    std::string s;
    s += Networking::MinPacketId;
    s += nlohmann::json{
      { "t", MsgType::ChangeValues },
      { "data",
        { { "health", health },
          { "magicka", magicka },
          { "stamina", stamina } } }
    }.dump();
    actor->SendToUser(s.data(), s.size(), true);
  }
  if (timeAfterRegeneration > 0.0f) {
  actor->SetPercentages(health, magicka, stamina);
  actor->SetLastAttributesPercentagesUpdate(now);
  }
}

namespace {

bool IsUnarmedAttack(const uint32_t sourceFormId)
{
  return sourceFormId == 0x1f4;
}

float CalculateCurrentHealthPercentage(const MpActor& actor, float damage,
                                       float healthPercentage)
{
  uint32_t baseId = actor.GetBaseId();
  uint32_t raceId = actor.GetRaceId();
  WorldState* espmProvider = actor.GetParent();
  float baseHealth = GetBaseActorValues(espmProvider, baseId, raceId).health;

  float damagePercentage = damage / baseHealth;
  float currentHealthPercentage = healthPercentage - damagePercentage;
  return currentHealthPercentage;
}

float GetReach(const MpActor& actor, const uint32_t source)
{
  auto espmProvider = actor.GetParent();
  if (IsUnarmedAttack(source)) {
    uint32_t raceId = actor.GetRaceId();
    return espm::GetData<espm::RACE>(raceId, espmProvider).unarmedReach;
  }
  auto weapDNAM = espm::GetData<espm::WEAP>(source, espmProvider).weapDNAM;
  float fCombatDistance =
    espm::GetData<espm::GMST>(espm::GMST::kFCombatDistance, espmProvider)
      .value;
  float weaponReach = weapDNAM ? weapDNAM->reach : 0;
  return weaponReach * fCombatDistance;
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
  float reach = GetReach(actor, hitData.source);
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
}

void ActionListener::OnHit(const RawMessageData& rawMsgData_,
                           const HitData& hitData_)
{
  auto currentHitTime = std::chrono::steady_clock::now();
  MpActor* aggressor = partOne.serverState.ActorByUser(rawMsgData_.userId);
  if (!aggressor) {
    throw std::runtime_error("Unable to change values without Actor attached");
  }

  if (aggressor->IsDead()) {
    spdlog::debug(fmt::format("{:x} actor is dead and can't attack",
                              aggressor->GetFormId()));
    return;
  }

  HitData hitData = hitData_;

  if (hitData.aggressor == 0x14) {
    hitData.aggressor = aggressor->GetFormId();
  } else {
    throw std::runtime_error("Events from non aggressor is not supported yet");
  }
  if (hitData.target == 0x14) {
    hitData.target = aggressor->GetFormId();
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

  auto& targetActor = partOne.worldState.GetFormAt<MpActor>(hitData.target);
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
    float distance = sqrtf(GetSqrDistanceToBounds(*aggressor, targetActor));
    float reach = GetReach(*aggressor, hitData.source);
    uint32_t aggressorId = aggressor->GetFormId();
    uint32_t targetId = targetActor.GetFormId();
    spdlog::debug(
      fmt::format("{:x} actor can't reach {:x} target because distance {} is "
                  "greater then first actor attack radius {}",
                  aggressorId, targetId, distance, reach));
    return;
  }

  MpChangeForm targetForm = targetActor.GetChangeForm();

  float healthPercentage = targetForm.healthPercentage;
  float magickaPercentage = targetForm.magickaPercentage;
  float staminaPercentage = targetForm.staminaPercentage;

  float damage = partOne.CalculateDamage(*aggressor, targetActor, hitData);
  damage = damage < 0.f ? 0.f : damage;
  float currentHealthPercentage =
    CalculateCurrentHealthPercentage(targetActor, damage, healthPercentage);

  currentHealthPercentage =
    currentHealthPercentage < 0.f ? 0.f : currentHealthPercentage;

  auto now = std::chrono::steady_clock::now();
  targetActor.NetSetPercentages(currentHealthPercentage, magickaPercentage,
                                staminaPercentage, now, aggressor);
  targetActor.SetLastHitTime(now);

  spdlog::debug("Target {0:x} is hitted by {1} damage. Current health "
                "percentage: {2}. Last "
                "health percentage: {3}. (Last: {3} => Current: {2})",
                hitData.target, damage, currentHealthPercentage,
                healthPercentage);
}

void ActionListener::OnUnknown(const RawMessageData& rawMsgData,
                               simdjson::dom::element data)
{
  spdlog::debug("Got unhandled message: {}", simdjson::minify(data));
}