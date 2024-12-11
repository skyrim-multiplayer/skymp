#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct HostMessage : public MessageBase<HostMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::Host)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("remoteId", remoteId);
  }

  uint64_t remoteId = 0;
};
