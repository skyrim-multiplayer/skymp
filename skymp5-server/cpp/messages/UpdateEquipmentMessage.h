#pragma once

#include "Inventory.h"
#include "MessageBase.h"
#include "MsgType.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <type_traits>

struct UpdateEquipmentMessage : public MessageBase<UpdateEquipmentMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char,
                           static_cast<char>(MsgType::UpdateEquipment)>{};

  using Data = nlohmann::json; // TODO: static typing

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("idx", idx)
      .Serialize("data", data);
  }

  uint32_t idx = 0;
  Data data;
};
