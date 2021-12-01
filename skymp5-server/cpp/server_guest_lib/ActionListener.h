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
  ActionListener(PartOne& partOne_)
    : partOne(partOne_)
  {
  }

  void OnCustomPacket(const RawMessageData& rawMsgData,
                      simdjson::dom::element& content) override;

  void OnUpdateMovement(const RawMessageData& rawMsgData, uint32_t idx,
                        const NiPoint3& pos, const NiPoint3& rot,
                        bool isInJumpState, bool isWeapDrawn,
                        uint32_t worldOrCell) override;

  void OnUpdateAnimation(const RawMessageData& rawMsgData,
                         uint32_t idx) override;

  void OnUpdateAppearance(const RawMessageData& rawMsgData, uint32_t idx,
                          const Appearance& appearance) override;

  void OnUpdateEquipment(const RawMessageData& rawMsgData, uint32_t idx,
                         simdjson::dom::element& data,
                         const Inventory& equipmentInv) override;

  void OnActivate(const RawMessageData& rawMsgData, uint32_t caster,
                  uint32_t target) override;

  void OnPutItem(const RawMessageData& rawMsgData, uint32_t target,
                 const Inventory::Entry& entry) override;

  void OnTakeItem(const RawMessageData& rawMsgData, uint32_t target,
                  const Inventory::Entry& entry) override;

  void OnFinishSpSnippet(const RawMessageData& rawMsgData, uint32_t snippetIdx,
                         simdjson::dom::element& returnValue) override;

  void OnEquip(const RawMessageData& rawMsgData, uint32_t baseId) override;

  void OnConsoleCommand(
    const RawMessageData& rawMsgData, const std::string& consoleCommandName,
    const std::vector<ConsoleCommands::Argument>& args) override;

  void OnCraftItem(const RawMessageData& rawMsgData,
                   const Inventory& inputObjects, uint32_t workbenchId,
                   uint32_t resultObjectId) override;

  void OnHostAttempt(const RawMessageData& rawMsgData,
                     uint32_t remoteId) override;

  void OnCustomEvent(const RawMessageData& rawMsgData, const char* eventName,
                     simdjson::dom::element& e) override;

  void OnChangeValues(const RawMessageData& rawMsgData,
                      const float healthPercentage,
                      const float magickaPercentage,
                      const float staminaPercentage) override;

  void OnHit(const RawMessageData& rawMsgData,
             const HitData& hitData) override;

  void OnUnknown(const RawMessageData& rawMsgData,
                 simdjson::dom::element data) override;

private:
  // Returns user's actor if exists
  MpActor* SendToNeighbours(uint32_t idx,
                            const simdjson::dom::element& jMessage,
                            Networking::UserId userId,
                            Networking::PacketData data, size_t length,
                            bool reliable);

  MpActor* SendToNeighbours(uint32_t idx, const RawMessageData& rawMsgData,
                            bool reliable = false);

  PartOne& partOne;
};
