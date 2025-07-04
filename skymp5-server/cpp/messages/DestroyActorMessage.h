#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct DestroyActorMessage : public MessageBase<DestroyActorMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::DestroyActor)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("idx", idx);
  }

  uint32_t idx = 0;
};
