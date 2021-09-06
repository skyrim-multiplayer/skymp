#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

enum class RunMode
{
  Standing,
  Walking,
  Running,
  Sprinting,
};

const inline std::string g_standing = "Standing";
const inline std::string g_walking = "Walking";
const inline std::string g_running = "Running";
const inline std::string g_sprinting = "Sprinting";

inline const std::string& ToString(RunMode runMode)
{
  switch (runMode) {
    case RunMode::Standing:
      return g_standing;
    case RunMode::Walking:
      return g_walking;
    case RunMode::Running:
      return g_running;
    case RunMode::Sprinting:
      return g_sprinting;
    default:
      throw std::runtime_error("unhandled case for RunMode");
  }
}

inline RunMode RunModeFromString(std::string_view str)
{
  if (str == g_standing) {
    return RunMode::Standing;
  } else if (str == g_walking) {
    return RunMode::Walking;
  } else if (str == g_running) {
    return RunMode::Running;
  } else if (str == g_sprinting) {
    return RunMode::Sprinting;
  } else {
    throw std::runtime_error("cannot parse RunMode from " + std::string{str});
  }
}

struct MovementData
{
  uint32_t idx;
  uint32_t worldOrCell;
  std::array<float, 3> pos;
  std::array<float, 3> rot;
  float direction;
  float healthPercentage;

  // flags & optionals
  RunMode runMode;
  bool isInJumpState;
  bool isSneaking;
  bool isBlocking;
  bool isWeapDrawn;
  std::optional<std::array<float, 3>> lookAt;

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
