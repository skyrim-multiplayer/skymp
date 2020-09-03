#pragma once
#include "Inventory.h"
#include "MpActor.h"             // Look
#include "NetworkingInterface.h" // UserId, PacketData
#include "NiPoint3.h"
#include <cstdint>
#include <simdjson.h>

class IActionListener
{
public:
  struct RawMessageData
  {
    Networking::PacketData unparsed = nullptr;
    size_t unparsedLength = 0;
    simdjson::dom::element parsed;
    Networking::UserId userId = Networking::InvalidUserId;
  };

  virtual void OnCustomPacket(const RawMessageData& rawMsgData,
                              simdjson::dom::element& content) = 0;

  virtual void OnUpdateMovement(const RawMessageData& rawMsgData, uint32_t idx,
                                const NiPoint3& pos, const NiPoint3& rot) = 0;

  virtual void OnUpdateAnimation(const RawMessageData& rawMsgData,
                                 uint32_t idx) = 0;

  virtual void OnUpdateLook(const RawMessageData& rawMsgData, uint32_t idx,
                            const MpActor::Look& look) = 0;

  virtual void OnUpdateEquipment(const RawMessageData& rawMsgData,
                                 uint32_t idx, simdjson::dom::element& data,
                                 const Inventory& equipmentInv) = 0;

  virtual void OnActivate(const RawMessageData& rawMsgData, uint32_t caster,
                          uint32_t target) = 0;

  virtual void OnPutItem(const RawMessageData& rawMsgData,
                         uint32_t target, const Inventory::Entry& entry) = 0;

  virtual void OnTakeItem(const RawMessageData& rawMsgData,
                          uint32_t target, const Inventory::Entry& entry) = 0;
};