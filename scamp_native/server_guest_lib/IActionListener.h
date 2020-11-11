#pragma once
#include "ConsoleCommands.h"
#include "Inventory.h"
#include "MpActor.h"             // Look
#include "NetworkingInterface.h" // UserId, PacketData
#include "NiPoint3.h"
#include <cstdint>
#include <simdjson.h>
#include <vector>

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
                              simdjson::dom::element& content)
  {
  }

  virtual void OnUpdateMovement(const RawMessageData& rawMsgData, uint32_t idx,
                                const NiPoint3& pos, const NiPoint3& rot,
                                bool isInJumpState, bool isWeapDrawn)
  {
  }

  virtual void OnUpdateAnimation(const RawMessageData& rawMsgData,
                                 uint32_t idx)
  {
  }

  virtual void OnUpdateLook(const RawMessageData& rawMsgData, uint32_t idx,
                            const Look& look)
  {
  }

  virtual void OnUpdateEquipment(const RawMessageData& rawMsgData,
                                 uint32_t idx, simdjson::dom::element& data,
                                 const Inventory& equipmentInv)
  {
  }

  virtual void OnActivate(const RawMessageData& rawMsgData, uint32_t caster,
                          uint32_t target)
  {
  }

  virtual void OnPutItem(const RawMessageData& rawMsgData, uint32_t target,
                         const Inventory::Entry& entry)
  {
  }

  virtual void OnTakeItem(const RawMessageData& rawMsgData, uint32_t target,
                          const Inventory::Entry& entry)
  {
  }

  virtual void OnFinishSpSnippet(const RawMessageData& rawMsgData,
                                 uint32_t snippetIdx,
                                 simdjson::dom::element& returnValue)
  {
  }

  virtual void OnEquip(const RawMessageData& rawMsgData, uint32_t baseId) {}

  virtual void OnConsoleCommand(
    const RawMessageData& rawMsgData, const std::string& consoleCommandName,
    const std::vector<ConsoleCommands::Argument>& args)
  {
  }

  virtual void OnCraftItem(const RawMessageData& rawMsgData,
                           const Inventory& inputObjects, uint32_t workbenchId,
                           uint32_t resultObjectId)
  {
  }
};