#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <array>

struct OpenContainerMessage : public MessageBase<OpenContainerMessage>
{
  const static char kMsgType = static_cast<char>(MsgType::OpenContainer);
  const static char kHeaderByte = static_cast<char>(MsgType::OpenContainer);

  void WriteBinary(SLNet::BitStream& stream) const override;
  void ReadBinary(SLNet::BitStream& stream) override;
  void WriteJson(nlohmann::json& json) const override;
  void ReadJson(const nlohmann::json& json) override;

  uint32_t target = 0;
};
