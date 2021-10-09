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

struct MovementMessage
{
  const static char kHeaderByte = 'M';

  uint32_t idx{};
  uint32_t worldOrCell{};
  std::array<float, 3> pos{ 0, 0, 0 };
  std::array<float, 3> rot{ 0, 0, 0 };
  float direction{};
  float healthPercentage{};

  // flags & optionals
  RunMode runMode{};
  bool isInJumpState{};
  bool isSneaking{};
  bool isBlocking{};
  bool isWeapDrawn{};
  std::optional<std::array<float, 3>> lookAt{ { 0, 0, 0 } };

  auto Tie() const
  {
    return std::tie(idx, worldOrCell, pos, rot, direction, healthPercentage,
                    runMode, isInJumpState, isSneaking, isBlocking,
                    isWeapDrawn, lookAt);
  }

  bool operator==(const MovementMessage& rhs) const
  {
    return Tie() == rhs.Tie();
  }
};
