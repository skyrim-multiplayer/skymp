#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <array>
#include <type_traits>

struct TeleportMessage : public MessageBase<TeleportMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::Teleport)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("idx", idx)
      .Serialize("pos", pos)
      .Serialize("rot", rot)
      .Serialize("worldOrCell", worldOrCell);
  }

  uint32_t idx = 0;
  std::array<float, 3> pos, rot;
  uint32_t worldOrCell = 0;
};
