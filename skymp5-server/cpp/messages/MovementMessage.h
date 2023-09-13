#pragma once

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

struct MovementMessage
{
  const static char kHeaderByte = 'M';

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

  auto Tie() const
  {
    return std::tie(idx, worldOrCell, pos, rot, direction, healthPercentage,
                    speed, runMode, isInJumpState, isSneaking, isBlocking,
                    isWeapDrawn, isDead, lookAt);
  }

  bool operator==(const MovementMessage& rhs) const
  {
    return Tie() == rhs.Tie();
  }
};
