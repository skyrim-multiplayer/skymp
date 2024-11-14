#pragma once

#include "MessageBase.h"
#include "MsgType.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <type_traits>

struct UpdatePropertyMessage : public MessageBase<UpdatePropertyMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::UpdateProperty)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("idx", idx)
      .Serialize("propName", propName)
      .Serialize("refrId", refrId)
      .Serialize("dataDump", dataDump)
      .Serialize("baseRecordType", baseRecordType);
  }

  uint32_t idx = 0;
  std::string propName;
  uint32_t refrId = 0;
  std::string dataDump;
  std::optional<std::string> baseRecordType;
};
