#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <optional>
#include <type_traits>

struct ActivateMessage : public MessageBase<ActivateMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::Activate)>{};

  struct Data
  {
    template <class Archive>
    void Serialize(Archive& archive)
    {
      archive.Serialize("caster", caster)
        .Serialize("target", target)
        .Serialize("isSecondActivation", isSecondActivation);
    }

    uint64_t caster = 0;
    uint64_t target = 0;
    bool isSecondActivation = false;
  };

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("data", data);
  }

  Data data;
};
