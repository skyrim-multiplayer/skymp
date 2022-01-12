#pragma once
#include "ConsoleCommands.h"
#include "HitData.h"
#include "Inventory.h"
#include "MpActor.h"             // Appearance
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
                                bool isInJumpState, bool isWeapDrawn,
                                uint32_t worldOrCell)
  {
  }

  virtual void OnUpdateAnimation(const RawMessageData& rawMsgData,
                                 uint32_t idx)
  {
  }

  virtual void OnUpdateAppearance(const RawMessageData& rawMsgData,
                                  uint32_t idx, const Appearance& appearance)
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

  virtual void OnHostAttempt(const RawMessageData& rawMsgData,
                             uint32_t remoteId)
  {
  }

  virtual void OnCustomEvent(const RawMessageData& rawMsgData,
                             const char* eventName, simdjson::dom::element& e)
  {
  }

  virtual void OnChangeValues(const RawMessageData& rawMsgData,
                              const float healthPercentage,
                              const float magickaPercentage,
                              const float staminaPercentage)
  {
  }

  virtual void OnHit(const RawMessageData& rawMsgData, const HitData& hitData)
  {
  }

  virtual void OnUnknown(const RawMessageData& rawMsgData,
                         simdjson::dom::element data)
  {
  }
};
