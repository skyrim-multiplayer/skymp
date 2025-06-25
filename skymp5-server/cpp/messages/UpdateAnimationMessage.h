#pragma once
#include "../server_guest_lib/AnimationData.h"
#include "MessageBase.h"
#include "MsgType.h"
#include <cstdint>
#include <string>
#include <type_traits>

struct UpdateAnimationMessage : public MessageBase<UpdateAnimationMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char,
                           static_cast<char>(MsgType::UpdateAnimation)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("idx", idx)
      .Serialize("data", data);
  }

  uint32_t idx = 0;
  AnimationData data;
};
