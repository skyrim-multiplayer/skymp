#pragma once
#include "MsgType.h"
#include <cstdint>
#include <string>
#include <type_traits>

struct UpdateAnimationMessage
{
  static constexpr auto kMsgType =
    std::integral_constant<char,
                           static_cast<char>(MsgType::UpdateAnimation)>{};

  struct Data
  {
    template <class Archive>
    void Serialize(Archive& archive)
    {
      archive.Serialize("numChanges", numChanges)
        .Serialize("animEventName", animEventName);
    }

    uint32_t numChanges = 0;
    std::string animEventName;
  };

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
