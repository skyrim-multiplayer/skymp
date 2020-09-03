#pragma once
#include "IActionListener.h"
#include "Loader.h"
#include "MpActor.h"
#include "PartOne.h"

class ServerState;
class WorldState;

class ActionListener : public IActionListener
{
public:
  ActionListener(
    WorldState& worldState_, ServerState& serverState_,
    std::vector<std::shared_ptr<PartOne::Listener>>& partOneListeners_,
    espm::Loader*& espm_, Networking::ISendTarget*& pushedSendTarget_)
    : worldState(worldState_)
    , serverState(serverState_)
    , partOneListeners(partOneListeners_)
    , espm(espm_)
    , pushedSendTarget(pushedSendTarget_)
  {
  }

  void OnCustomPacket(const RawMessageData& rawMsgData,
                      simdjson::dom::element& content) override;

  void OnUpdateMovement(const RawMessageData& rawMsgData, uint32_t idx,
                        const NiPoint3& pos, const NiPoint3& rot) override;

  void OnUpdateAnimation(const RawMessageData& rawMsgData,
                         uint32_t idx) override;

  void OnUpdateLook(const RawMessageData& rawMsgData, uint32_t idx,
                    const MpActor::Look& look) override;

  void OnUpdateEquipment(const RawMessageData& rawMsgData, uint32_t idx,
                         simdjson::dom::element& data,
                         const Inventory& equipmentInv) override;

  void OnActivate(const RawMessageData& rawMsgData, uint32_t caster,
                  uint32_t target) override;

  void OnPutItem(const RawMessageData& rawMsgData, uint32_t target,
                 const Inventory::Entry& entry) override;

  void OnTakeItem(const RawMessageData& rawMsgData, uint32_t target,
                  const Inventory::Entry& entry) override;

private:
  // Returns user's actor if exists
  MpActor* SendToNeighbours(uint32_t idx,
                            const simdjson::dom::element& jMessage,
                            Networking::UserId userId,
                            Networking::PacketData data, size_t length,
                            bool reliable);

  MpActor* SendToNeighbours(uint32_t idx, const RawMessageData& rawMsgData,
                            bool reliable = false);

  WorldState& worldState;
  ServerState& serverState;
  std::vector<std::shared_ptr<PartOne::Listener>>& partOneListeners;
  espm::Loader*& espm;
  Networking::ISendTarget*& pushedSendTarget;
};