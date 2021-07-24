#include "ActionListener.h"
#include "DummyMessageOutput.h"
#include "EspmGameObject.h"
#include "Exceptions.h"
#include "FindRecipe.h"
#include "MovementValidation.h"
#include "MsgType.h"
#include "PapyrusObjectReference.h"
#include "UserMessageOutput.h"
#include "Utils.h"

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

    if (!MovementValidation::Validate(
          *actor, teleportFlag ? reallyWrongPos : pos, worldOrCell,
          isMe ? static_cast<IMessageOutput&>(msgOutput)
               : static_cast<IMessageOutput&>(msgOutputDummy))) {
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

void ActionListener::OnUpdateLook(const RawMessageData& rawMsgData,
                                  uint32_t idx, const Look& look)
{ // TODO: validate

  MpActor* actor = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!actor || !actor->IsRaceMenuOpen())
    return;

  actor->SetRaceMenuOpen(false);
  actor->SetLook(&look);
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

Equipment GetEquipment(MpActor& ac)
{
  std::string equipment = ac.GetEquipmentAsJson();
  simdjson::dom::parser p;
  auto parseResult = p.parse(equipment);
  return Equipment::FromJson(parseResult.value());
}

void RecalculateWorn(MpObjectReference& refr)
{
  if (!refr.GetParent()->HasEspm())
    return;
  auto& loader = refr.GetParent()->GetEspm();

  auto ac = dynamic_cast<MpActor*>(&refr);
  if (!ac)
    return;

  const Equipment eq = GetEquipment(*ac);

  Equipment newEq;
  newEq.numChanges = eq.numChanges + 1;
  for (auto& entry : eq.inv.entries) {
    bool isEquipped = entry.extra.worn != Inventory::Worn::None;
    bool isWeap = !!espm::Convert<espm::WEAP>(
      loader.GetBrowser().LookupById(entry.baseId).rec);
    if (isEquipped && isWeap)
      continue;
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
            weap->GetData().weapData->damage > bestDamage) {
          bestEntry = entry;
          bestDamage = weap->GetData().weapData->damage;
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
    if (!actor)
      continue;
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
  auto mapping = br.GetMapping(espmIdx);
  std::vector<Inventory::Entry> entries;
  for (auto& entry : recipeData.inputObjects) {
    auto formId = espm::GetMappedId(entry.formId, *mapping);
    entries.push_back({ formId, entry.count });
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
  auto base = br.LookupById(workbench.GetBaseId());

  if (base.rec->GetType() != "FURN")
    throw std::runtime_error("Unable to use " +
                             base.rec->GetType().ToString() + " as workbench");

  int espmIdx = 0;
  auto recipeUsed = FindRecipe(br, inputObjects, resultObjectId, &espmIdx);

  if (!recipeUsed)
    throw std::runtime_error("Recipe not found");

  MpActor* me = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!me)
    throw std::runtime_error("Unable to craft without Actor attached");

  auto recipeData = recipeUsed->GetData();
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

  if (hoster == 0 || !lastRemoteUpdate ||
      std::chrono::system_clock::now() - *lastRemoteUpdate >
        hostResetTimeout) {
    partOne.GetLogger().info("Hoster changed from {0:x} to {0:x}", prevHoster,
                             me->GetFormId());
    hoster = me->GetFormId();
    remote.UpdateHoster(hoster);
    RecalculateWorn(remote);

    uint64_t longFormId = remote.GetFormId();
    if (dynamic_cast<MpActor*>(&remote) && longFormId < 0xff000000) {
      longFormId += 0x100000000;
    }

    Networking::SendFormatted(&partOne.GetSendTarget(), rawMsgData.userId,
                              R"({ "type": "hostStart", "target": %llu })",
                              longFormId);

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
  if (!ac)
    return;

  if (eventName[0] != '_')
    return;

  for (auto& listener : partOne.GetListeners()) {
    listener->OnMpApiEvent(eventName, args, ac->GetFormId());
  }
}
