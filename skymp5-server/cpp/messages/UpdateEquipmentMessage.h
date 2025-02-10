#pragma once

#include "../server_guest_lib/Equipment.h"
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

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("idx", idx)
      .Serialize("data", data);
  }

  uint32_t idx = 0;
  Equipment data;
};
