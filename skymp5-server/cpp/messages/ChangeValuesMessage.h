#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <optional>

struct ChangeValuesMessage : public MessageBase<ChangeValuesMessage>
{
  const static char kMsgType = static_cast<char>(MsgType::ChangeValues);
  const static char kHeaderByte = static_cast<char>(MsgType::ChangeValues);

  void WriteBinary(SLNet::BitStream& stream) const override;
  void ReadBinary(SLNet::BitStream& stream) override;
  void WriteJson(nlohmann::json& json) const override;
  void ReadJson(const nlohmann::json& json) override;

  std::optional<uint32_t> idx;

  // percentages
  float health = 0;
  float magicka = 0;
  float stamina = 0;
};
