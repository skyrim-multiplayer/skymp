#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <array>
#include <type_traits>

struct Teleport2Message : public MessageBase<Teleport2Message>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::Teleport2)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("pos", pos)
      .Serialize("rot", rot)
      .Serialize("worldOrCell", worldOrCell);
  }

  std::array<float, 3> pos, rot;
  uint32_t worldOrCell = 0;
};
