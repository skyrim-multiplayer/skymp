#pragma once

#include <array>
#include <optional>

enum class RunMode
{
  Standing,
  Walking,
  Running,
  Sprinting,
};

struct MovementData
{
  uint32_t idx;
  uint32_t worldOrCell;
  std::array<float, 3> pos;
  std::array<float, 3> rot;
  std::optional<std::array<float, 3>> lookAt;
  float direction;
  float healthPercentage;

  // packed flags
  RunMode runMode;
  bool isInJumpState;
  bool isSneaking;
  bool isBlocking;
  bool isWeapDrawn;

  uint8_t GetPackedFlags() const {
    uint8_t result = 0;
    result = static_cast<uint8_t>(runMode);
    result |= isInJumpState << 2;
    result |= isSneaking << 3;
    result |= isBlocking << 4;
    result |= isWeapDrawn << 5;
    return result;
  }

  void SetPackedFlags(uint8_t flags) {
    runMode = static_cast<RunMode>(flags & 3);
    isInJumpState = static_cast<bool>((flags >> 2) & 1);
    isSneaking = static_cast<bool>((flags >> 3) & 1);
    isBlocking = static_cast<bool>((flags >> 4) & 1);
    isWeapDrawn = static_cast<bool>((flags >> 5) & 1);
  }
};
