#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct CustomEventMessage : public MessageBase<CustomEventMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::CustomEvent)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("argsJsonDumps", argsJsonDumps)
      .Serialize("eventName", eventName);
  }

  std::vector<std::string> argsJsonDumps;
  std::string eventName;
};
