#pragma once
#include "MessageBase.h"
#include "MsgType.h"

#include "TeleportMessage.h"
#include "ChangeValuesMessage.h"
#include "UpdatePropertyMessage.h"

#include <optional>

struct DeathStateContainerMessage : public MessageBase<DeathStateContainerMessage>
{
  const static char kMsgType = static_cast<char>(MsgType::DeathStateContainer);
  const static char kHeaderByte =
    static_cast<char>(MsgType::DeathStateContainer);

  void WriteBinary(SLNet::BitStream& stream) const override;
  void ReadBinary(SLNet::BitStream& stream) override;
  void WriteJson(nlohmann::json& json) const override;
  void ReadJson(const nlohmann::json& json) override;

  std::optional<TeleportMessage> tTeleport;
  std::optional<ChangeValuesMessage> tChangeValues;
  std::optional<UpdatePropertyMessage> tIsDead;
};
