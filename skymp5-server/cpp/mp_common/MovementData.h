#pragma once

#include <array>
#include <optional>
#include <stdexcept>
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

// XXX: move to cpp
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

// XXX: rename to MovementMessage?
struct MovementData
{
  const static char kHeaderByte = 'M';

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
  
  auto Tie() const {
    return std::tie(idx, worldOrCell, pos, rot, direction, healthPercentage, runMode, isInJumpState,
        isSneaking, isBlocking, isWeapDrawn, lookAt);
  }

  bool operator==(const MovementData& rhs) const {
    return Tie() == rhs.Tie();
  }
};
