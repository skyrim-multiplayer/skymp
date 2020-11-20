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
  MpActor* myActor = partOne.serverState.ActorByUser(userId);
  if (!myActor)
    throw std::runtime_error(
      "SendToNeighbours - no actor is attached to user");

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
                                      bool isWeapDrawn)
{
  auto actor = SendToNeighbours(idx, rawMsgData);
  if (actor) {
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

void ExecuteAddItem(MpActor& caller,
                    const std::vector<ConsoleCommands::Argument>& args)
{
  const auto targetId = static_cast<uint32_t>(args.at(0).GetInteger());
  const auto itemId = static_cast<uint32_t>(args.at(1).GetInteger());
  const auto count = static_cast<int32_t>(args.at(2).GetInteger());

  MpObjectReference& target = (targetId == 0x14)
    ? caller
    : caller.GetParent()->GetFormAt<MpObjectReference>(targetId);

  auto& br = caller.GetParent()->GetEspm().GetBrowser();

  PapyrusObjectReference papyrusObjectReference;
  auto aItem =
    VarValue(std::make_shared<EspmGameObject>(br.LookupById(itemId)));
  auto aCount = VarValue(count);
  auto aSilent = VarValue(false);
  (void)papyrusObjectReference.AddItem(target.ToVarValue(),
                                       { aItem, aCount, aSilent });
}

void ExecutePlaceAtMe(MpActor& caller,
                      const std::vector<ConsoleCommands::Argument>& args)
{
  const auto targetId = static_cast<uint32_t>(args.at(0).GetInteger());
  const auto baseFormId = static_cast<uint32_t>(args.at(1).GetInteger());

  MpObjectReference& target = (targetId == 0x14)
    ? caller
    : caller.GetParent()->GetFormAt<MpObjectReference>(targetId);

  auto& br = caller.GetParent()->GetEspm().GetBrowser();

  PapyrusObjectReference papyrusObjectReference;
  auto aBaseForm =
    VarValue(std::make_shared<EspmGameObject>(br.LookupById(baseFormId)));
  auto aCount = VarValue(1);
  auto aForcePersist = VarValue(false);
  auto aInitiallyDisabled = VarValue(false);
  (void)papyrusObjectReference.PlaceAtMe(
    target.ToVarValue(),
    { aBaseForm, aCount, aForcePersist, aInitiallyDisabled });
}

void ExecuteDisable(MpActor& caller,
                    const std::vector<ConsoleCommands::Argument>& args)
{
  const auto targetId = static_cast<uint32_t>(args.at(0).GetInteger());

  MpObjectReference& target = (targetId == 0x14)
    ? caller
    : caller.GetParent()->GetFormAt<MpObjectReference>(targetId);

  target.Disable();
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
    ExecuteAddItem(*me, args);
  } else if (!Utils::stricmp(consoleCommandName.data(), "PlaceAtMe")) {
    ExecutePlaceAtMe(*me, args);
  } else if (!Utils::stricmp(consoleCommandName.data(), "Disable")) {
    ExecuteDisable(*me, args);
  } else
    throw std::runtime_error("Unknown command name '" + consoleCommandName +
                             "'");
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

    Networking::SendFormatted(&partOne.GetSendTarget(), rawMsgData.userId,
                              R"({ "type": "hostStart", "target": %u })",
                              remote.GetFormId());

    if (MpActor* prevHosterActor = dynamic_cast<MpActor*>(
          partOne.worldState.LookupFormById(prevHoster).get())) {
      auto prevHosterUser = partOne.serverState.UserByActor(prevHosterActor);
      if (prevHosterUser != Networking::InvalidUserId &&
          prevHosterUser != rawMsgData.userId) {
        Networking::SendFormatted(&partOne.GetSendTarget(), prevHosterUser,
                                  R"({ "type": "hostStop", "target": %u })",
                                  remote.GetFormId());
      }
    }
  }
}