#pragma once

#include "../server_guest_lib/VoiceChat.h"
#include "MessageBase.h"
#include "MsgType.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <type_traits>

struct UpdateVoiceChatMessage : public MessageBase<UpdateVoiceChatMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char,
                           static_cast<char>(MsgType::UpdateVoiceChatMessage)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("idx", idx)
      .Serialize("data", data);
  }

  uint32_t idx = 0;
  VoiceChat data;
};
