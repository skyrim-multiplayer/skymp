#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

#include "Inventory.h"

struct SetInventoryMessage : public MessageBase<SetInventoryMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::SetInventory)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("inventory", inventory);
  }

  Inventory inventory;
};
