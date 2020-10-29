#include "ActionListener.h"
#include "Exceptions.h"

MpActor* ActionListener::SendToNeighbours(
  uint32_t idx, const simdjson::dom::element& jMessage,
  Networking::UserId userId, Networking::PacketData data, size_t length,
  bool reliable)
{
  MpActor* actor = serverState.ActorByUser(userId);

  if (actor) {
    if (idx != actor->GetIdx()) {
      std::stringstream ss;
      ss << std::hex << "You aren't able to update actor with idx " << idx
         << " (your actor's idx is " << actor->GetIdx() << ')';
      throw PublicError(ss.str());
    }

    for (auto listener : actor->GetListeners()) {
      auto listenerAsActor = dynamic_cast<MpActor*>(listener);
      if (listenerAsActor) {
        auto targetuserId = serverState.UserByActor(listenerAsActor);
        if (targetuserId != Networking::InvalidUserId)
          pushedSendTarget->Send(targetuserId, data, length, reliable);
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
  for (auto& listener : partOneListeners)
    listener->OnCustomPacket(rawMsgData.userId, content);
}

void ActionListener::OnUpdateMovement(const RawMessageData& rawMsgData,
                                      uint32_t idx, const NiPoint3& pos,
                                      const NiPoint3& rot, bool isInJumpState,
                                      bool isWeapDrawn)
{
  auto actor = SendToNeighbours(idx, rawMsgData);
  if (actor) {
    actor->SetPos(pos);
    actor->SetAngle(rot);
    actor->SetAnimationVariableBool("bInJumpState", isInJumpState);
    actor->SetAnimationVariableBool("_skymp_isWeapDrawn", isWeapDrawn);
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

  MpActor* actor = serverState.ActorByUser(rawMsgData.userId);
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
  MpActor* actor = serverState.ActorByUser(rawMsgData.userId);
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

void ActionListener::OnActivate(const RawMessageData& rawMsgData,
                                uint32_t caster, uint32_t target)
{
  if (!espm)
    throw std::runtime_error("No loaded esm or esp files are found");

  const auto ac = serverState.ActorByUser(rawMsgData.userId);
  if (!ac)
    throw std::runtime_error("Can't do this without Actor attached");

  if (caster != 0x14) {
    std::stringstream ss;
    ss << "Bad caster (0x" << std::hex << caster << ")";
    throw std::runtime_error(ss.str());
  }

  auto refPtr = std::dynamic_pointer_cast<MpObjectReference>(
    worldState.LookupFormById(target));
  if (!refPtr)
    return;

  refPtr->Activate(*ac);
}

void ActionListener::OnPutItem(const RawMessageData& rawMsgData,
                               uint32_t target, const Inventory::Entry& entry)
{
  MpActor* actor = serverState.ActorByUser(rawMsgData.userId);
  auto& ref = worldState.GetFormAt<MpObjectReference>(target);

  if (!actor || !espm)
    return; // TODO: Throw error instead
  ref.PutItem(*actor, entry);
}

void ActionListener::OnTakeItem(const RawMessageData& rawMsgData,
                                uint32_t target, const Inventory::Entry& entry)
{
  MpActor* actor = serverState.ActorByUser(rawMsgData.userId);
  auto& ref = worldState.GetFormAt<MpObjectReference>(target);

  if (!actor || !espm)
    return; // TODO: Throw error instead
  ref.TakeItem(*actor, entry);
}

namespace {
VarValue VarValueFromJson(const simdjson::dom::element& parentMsg,
                          const simdjson::dom::element& element)
{
  auto key = "returnValue";

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
  MpActor* actor = serverState.ActorByUser(rawMsgData.userId);
  if (!actor)
    throw std::runtime_error(
      "Unable to finish SpSnippet: No Actor found for user " +
      std::to_string(rawMsgData.userId));

  actor->ResolveSnippet(snippetIdx,
                        VarValueFromJson(rawMsgData.parsed, returnValue));
}

void ActionListener::OnEquip(const RawMessageData& rawMsgData, uint32_t baseId)
{
  MpActor* actor = serverState.ActorByUser(rawMsgData.userId);
  if (!actor)
    throw std::runtime_error(
      "Unable to finish SpSnippet: No Actor found for user " +
      std::to_string(rawMsgData.userId));

  actor->OnEquip(baseId);
}