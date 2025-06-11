#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct CustomPacketMessage : public MessageBase<CustomPacketMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::CustomPacket)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("contentJsonDump", contentJsonDump);
  }

  std::string contentJsonDump;
};
