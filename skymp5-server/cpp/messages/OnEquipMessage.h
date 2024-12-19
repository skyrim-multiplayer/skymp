#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct OnEquipMessage : public MessageBase<OnEquipMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::OnEquip)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("baseId", baseId);
  }

  uint32_t baseId = 0;
};
