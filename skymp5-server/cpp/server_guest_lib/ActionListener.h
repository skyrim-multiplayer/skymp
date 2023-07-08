#pragma once
#include "ActionListener.h"
#include "AnimationData.h"
#include "ConsoleCommands.h"
#include "MpActor.h"
#include "PartOne.h"
#include "libespm/Loader.h"

class ServerState;
class WorldState;
struct ActorValues;

class ActionListener
{
public:
  struct RawMessageData
  {
    Networking::PacketData unparsed = nullptr;
    size_t unparsedLength = 0;
    simdjson::dom::element parsed;
    Networking::UserId userId = Networking::InvalidUserId;
  };

  ActionListener(PartOne& partOne_)
    : partOne(partOne_)
  {
  }

  virtual void OnCustomPacket(const RawMessageData& rawMsgData,
                              simdjson::dom::element& content);

  virtual void OnUpdateMovement(const RawMessageData& rawMsgData, uint32_t idx,
                                const NiPoint3& pos, const NiPoint3& rot,
                                bool isInJumpState, bool isWeapDrawn,
                                bool isBlocking, uint32_t worldOrCell);

  virtual void OnUpdateAnimation(const RawMessageData& rawMsgData,
                                 uint32_t idx,
                                 const AnimationData& animationData);

  virtual void OnUpdateAppearance(const RawMessageData& rawMsgData,
                                  uint32_t idx, const Appearance& appearance);

  virtual void OnUpdateEquipment(const RawMessageData& rawMsgData,
                                 uint32_t idx,
                                 const simdjson::dom::element& data,
                                 const Inventory& equipmentInv,
                                 uint32_t leftSpell, uint32_t rightSpell,
                                 uint32_t voiceSpell, uint32_t instantSpell);

  virtual void OnActivate(const RawMessageData& rawMsgData, uint32_t caster,
                          uint32_t target);

  virtual void OnPutItem(const RawMessageData& rawMsgData, uint32_t target,
                         const Inventory::Entry& entry);

  virtual void OnTakeItem(const RawMessageData& rawMsgData, uint32_t target,
                          const Inventory::Entry& entry);

  virtual void OnDropItem(const RawMessageData& rawMsgdata, uint32_t baseId,
                          const Inventory::Entry& entry);

  virtual void OnFinishSpSnippet(const RawMessageData& rawMsgData,
                                 uint32_t snippetIdx,
                                 simdjson::dom::element& returnValue);

  virtual void OnEquip(const RawMessageData& rawMsgData, uint32_t baseId);

  virtual void OnConsoleCommand(
    const RawMessageData& rawMsgData, const std::string& consoleCommandName,
    const std::vector<ConsoleCommands::Argument>& args);

  virtual void OnCraftItem(const RawMessageData& rawMsgData,
                           const Inventory& inputObjects, uint32_t workbenchId,
                           uint32_t resultObjectId);

  virtual void OnHostAttempt(const RawMessageData& rawMsgData,
                             uint32_t remoteId);

  virtual void OnCustomEvent(const RawMessageData& rawMsgData,
                             const char* eventName, simdjson::dom::element& e);

  virtual void OnChangeValues(const RawMessageData& rawMsgData,
                              const ActorValues& actorValues);

  virtual void OnHit(const RawMessageData& rawMsgData, const HitData& hitData);

  virtual void OnUnknown(const RawMessageData& rawMsgData,
                         simdjson::dom::element data);

private:
  // Returns user's actor if there is attached one
  MpActor* SendToNeighbours(uint32_t idx,
                            const simdjson::dom::element& jMessage,
                            Networking::UserId userId,
                            Networking::PacketData data, size_t length,
                            bool reliable);

  MpActor* SendToNeighbours(uint32_t idx, const RawMessageData& rawMsgData,
                            bool reliable = false);

  PartOne& partOne;
};
