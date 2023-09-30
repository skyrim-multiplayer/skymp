#pragma once

#include "MessageBase.h"
#include "MsgType.h"
#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

struct UpdateEquipmentMessage : public MessageBase<UpdateEquipmentMessage>
{
  const static char kMsgType = static_cast<char>(MsgType::UpdateEquipment);
  const static char kHeaderByte = static_cast<char>(MsgType::UpdateEquipment);

  void WriteBinary(SLNet::BitStream& stream) const override;
  void ReadBinary(SLNet::BitStream& stream) override;
  void WriteJson(nlohmann::json& json) const override;
  void ReadJson(const nlohmann::json& json) override;

  uint32_t idx = 0;
  nlohmann::json data; // TODO: static typing
};
