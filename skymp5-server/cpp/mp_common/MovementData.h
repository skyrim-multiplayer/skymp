#pragma once

#include <array>
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
