#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <array>

struct TeleportMessage : public MessageBase<TeleportMessage>
{
  const static char kMsgType = static_cast<char>(MsgType::Teleport);
  const static char kHeaderByte = static_cast<char>(MsgType::Teleport);

  void WriteBinary(SLNet::BitStream& stream) const override;
  void ReadBinary(SLNet::BitStream& stream) override;
  void WriteJson(nlohmann::json& json) const override;
  void ReadJson(const nlohmann::json& json) override;

  uint32_t idx = 0;

  std::array<float, 3> pos, rot;
  uint32_t worldOrCell = 0;
};
