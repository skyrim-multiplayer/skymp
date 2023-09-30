#pragma once

#include "MessageBase.h"
#include "MsgType.h"
#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

struct UpdatePropertyMessage : public MessageBase<UpdatePropertyMessage>
{
  const static char kMsgType = static_cast<char>(MsgType::UpdateProperty);
  const static char kHeaderByte = static_cast<char>(MsgType::UpdateProperty);

  void WriteBinary(SLNet::BitStream& stream) const override;
  void ReadBinary(SLNet::BitStream& stream) override;
  void WriteJson(nlohmann::json& json) const override;
  void ReadJson(const nlohmann::json& json) override;

  uint32_t idx = 0;
  std::string propName;
  uint32_t refrId = 0;
  nlohmann::json data;
  std::optional<std::string> baseRecordType;
};
