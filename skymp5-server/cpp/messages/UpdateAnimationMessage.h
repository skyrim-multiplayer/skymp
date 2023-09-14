#pragma once

#include <cstdint>
#include <string>
#include "MsgType.h"
#include "MessageBase.h"

struct UpdateAnimationMessage : public MessageBase {
  const static char kMsgType = static_cast<char>(MsgType::UpdateAnimation);
  const static char kHeaderByte = static_cast<char>(MsgType::UpdateAnimation);

  void WriteBinary(SLNet::BitStream& stream) const override;
  void ReadBinary(SLNet::BitStream& stream) override;
  void WriteJson(nlohmann::json &json) const override;
  void ReadJson(const nlohmann::json &json) override;

  uint32_t idx = 0;
  uint32_t numChanges = 0;
  std::string animEventName;
};
