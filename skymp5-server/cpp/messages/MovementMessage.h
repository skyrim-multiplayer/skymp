#pragma once

#include "MessageBase.h"
#include "MsgType.h"
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

enum class RunMode
{
  Standing,
  Walking,
  Running,
  Sprinting,
};

const std::string& ToString(RunMode runMode);

RunMode RunModeFromString(std::string_view str);

struct MovementMessage : public MessageBase
{
  const static char kMsgType = static_cast<char>(MsgType::UpdateMovement);
  const static char kHeaderByte = 'M';

  void WriteBinary(SLNet::BitStream& stream) const override;
  void ReadBinary(SLNet::BitStream& stream) override;
  void WriteJson(nlohmann::json& json) const override;
  void ReadJson(const nlohmann::json& json) override;

  uint32_t idx = 0;
  uint32_t worldOrCell = 0;
  std::array<float, 3> pos{ 0, 0, 0 };
  std::array<float, 3> rot{ 0, 0, 0 };
  float direction = 0;
  float healthPercentage = 0;
  float speed = 0;

  // flags & optionals
  RunMode runMode = RunMode::Standing;
  bool isInJumpState = false;
  bool isSneaking = false;
  bool isBlocking = false;
  bool isWeapDrawn = false;
  bool isDead = false;
  std::optional<std::array<float, 3>> lookAt = std::nullopt;
};
