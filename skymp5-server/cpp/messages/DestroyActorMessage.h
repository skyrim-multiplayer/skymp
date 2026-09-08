#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <cstdint>
#include <type_traits>

struct DestroyActorMessage : public MessageBase<DestroyActorMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::DestroyActor)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("idx", idx)
      .Serialize("refrId", refrId);
  }

  uint32_t idx = 0;
  uint64_t refrId = 0;
};
