#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct DropItemMessage : public MessageBase<DropItemMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::DropItem)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("baseId", baseId)
      .Serialize("count", count);
  }

  uint32_t baseId = 0;
  uint32_t count = 0;
};
