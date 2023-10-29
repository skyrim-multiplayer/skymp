#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <array>
#include <type_traits>

struct OpenContainerMessage : public MessageBase<OpenContainerMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::OpenContainer)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("target", target);
  }

  uint32_t target = 0;
};
