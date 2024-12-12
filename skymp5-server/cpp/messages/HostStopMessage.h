#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct HostStopMessage : public MessageBase<HostStopMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::HostStop)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("target", target);
  }

  uint64_t target = 0;
};
