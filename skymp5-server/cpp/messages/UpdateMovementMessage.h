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
#include <type_traits>

struct UpdateMovementMessage : public MessageBase<UpdateMovementMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::UpdateMovement)>{};

  struct Data
  {
    template <class Archive>
    void Serialize(Archive& archive)
    {
      archive.Serialize("worldOrCell", worldOrCell)
        .Serialize("pos", pos)
        .Serialize("rot", rot)
        .Serialize("direction", direction)
        .Serialize("healthPercentage", healthPercentage)
        .Serialize("speed", speed)
        .Serialize("runMode", runMode)
        .Serialize("isInJumpState", isInJumpState)
        .Serialize("isSneaking", isSneaking)
        .Serialize("isBlocking", isBlocking)
        .Serialize("isWeapDrawn", isWeapDrawn)
        .Serialize("isDead", isDead)
        .Serialize("lookAt", lookAt);
    }

    uint32_t worldOrCell = 0;
    std::array<float, 3> pos{ 0, 0, 0 };
    std::array<float, 3> rot{ 0, 0, 0 };
    float direction = 0;
    float healthPercentage = 0;
    float speed = 0;

    // flags & optionals
    std::string runMode = "Standing";
    bool isInJumpState = false;
    bool isSneaking = false;
    bool isBlocking = false;
    bool isWeapDrawn = false;
    bool isDead = false;
    std::optional<std::array<float, 3>> lookAt = std::nullopt;
  };

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("idx", idx)
      .Serialize("data", data);
  }

  uint32_t idx = 0;
  Data data;
};
