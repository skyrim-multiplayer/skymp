#include "ActionListener.h"
#include "EspmGameObject.h"
#include "Exceptions.h"
#include "FindRecipe.h"
#include "PapyrusObjectReference.h"
#include "Utils.h"

MpActor* ActionListener::SendToNeighbours(
  uint32_t idx, const simdjson::dom::element& jMessage,
  Networking::UserId userId, Networking::PacketData data, size_t length,
  bool reliable)
{
  MpActor* actor = partOne.serverState.ActorByUser(userId);

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
        auto targetuserId = partOne.serverState.UserByActor(listenerAsActor);
        if (targetuserId != Networking::InvalidUserId) {
          partOne.GetSendTarget().Send(targetuserId, data, length, reliable);
        }
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

void ActionListener::OnActivate(const RawMessageData& rawMsgData,
                                uint32_t caster, uint32_t target)
{
  if (!partOne.HasEspm())
    throw std::runtime_error("No loaded esm or esp files are found");

  const auto ac = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!ac)
    throw std::runtime_error("Can't do this without Actor attached");

  if (caster != 0x14) {
    std::stringstream ss;
    ss << "Bad caster (0x" << std::hex << caster << ")";
    throw std::runtime_error(ss.str());
  }

  auto refPtr = std::dynamic_pointer_cast<MpObjectReference>(
    partOne.worldState.LookupFormById(target));
  if (!refPtr)
    return;

  refPtr->Activate(*ac, false);
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
  int profileId = me ? me->GetChangeForm().profileId : -1;
  if (profileId != MpActor::kProfileId_Pospelov)
    throw std::runtime_error("Not enough permissions to use this command");

  if (!Utils::stricmp(consoleCommandName.data(), "AddItem")) {
    const auto targetId = static_cast<uint32_t>(args[0].GetInteger());
    const auto itemId = static_cast<uint32_t>(args[1].GetInteger());
    const auto count = static_cast<int32_t>(args[2].GetInteger());

    MpObjectReference* target = nullptr;
    if (targetId == 0x14) {
      target = partOne.serverState.ActorByUser(rawMsgData.userId);
      if (!target)
        throw std::runtime_error("No actor found for user " +
                                 std::to_string(rawMsgData.userId));
    } else
      target = &partOne.worldState.GetFormAt<MpObjectReference>(targetId);

    auto& br = partOne.GetEspm().GetBrowser();

    PapyrusObjectReference papyrusObjectReference;
    std::vector<VarValue> args;
    args.push_back(
      VarValue(std::make_shared<EspmGameObject>(br.LookupById(itemId))));
    args.push_back(VarValue(count));
    args.push_back(VarValue(false));
    papyrusObjectReference.AddItem(target->ToVarValue(), args);
  }
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