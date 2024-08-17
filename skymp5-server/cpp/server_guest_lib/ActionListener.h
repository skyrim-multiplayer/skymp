#pragma once
#include "ActionListener.h"
#include "AnimationData.h"
#include "ConsoleCommands.h"
#include "MovementMessage.h" // RunMode
#include "MpActor.h"
#include "PartOne.h"
#include "libespm/Loader.h"
#include <BS_thread_pool.hpp>
#include <list>

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

  explicit ActionListener(PartOne& partOne_);

  virtual void OnCustomPacket(const RawMessageData& rawMsgData,
                              simdjson::dom::element& content);

  virtual void OnUpdateMovement(const RawMessageData& rawMsgData, uint32_t idx,
                                const NiPoint3& pos, const NiPoint3& rot,
                                bool isInJumpState, bool isWeapDrawn,
                                bool isBlocking, uint32_t worldOrCell,
                                RunMode runMode);

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

  virtual void OnPlayerBowShot(const RawMessageData& rawMsgdata,
                               uint32_t weaponId, uint32_t ammoId, float power,
                               bool isSunGazing);

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

  virtual void OnUnknown(const RawMessageData& rawMsgData);

  virtual void TickDeferredSendToNeighboursMultithreaded();

private:
  // Returns user's actor if there is attached one
  MpActor* SendToNeighbours(uint32_t idx, MpActor* myActor,
                            Networking::PacketData data, size_t length,
                            bool reliable);

  MpActor* SendToNeighbours(uint32_t idx, const RawMessageData& rawMsgData,
                            bool reliable = false);

  PartOne& partOne;

  struct DeferredSendToNeighboursEntry
  {
    DeferredSendToNeighboursEntry(DeferredSendToNeighboursEntry&& rhs) =
      default;
    DeferredSendToNeighboursEntry& operator=(
      DeferredSendToNeighboursEntry&& rhs) = default;

    uint32_t idx = -1;
    MpActor* myActor = nullptr;
    std::vector<uint8_t> rawMsgCopy;
  };

  std::list<DeferredSendToNeighboursEntry> deferredSendToNeighbours;

  BS::thread_pool threadPool;

  std::vector<std::future<void>> futures;
};
