#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct HostStartMessage : public MessageBase<HostStartMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::HostStart)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("target", target);
  }

  uint64_t target = 0;
};
