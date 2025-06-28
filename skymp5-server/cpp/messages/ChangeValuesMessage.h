#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <optional>
#include <type_traits>

struct ChangeValuesMessage : public MessageBase<ChangeValuesMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::ChangeValues)>{};

  struct Data
  {
    template <class Archive>
    void Serialize(Archive& archive)
    {
      archive.Serialize("health", health)
        .Serialize("magicka", magicka)
        .Serialize("stamina", stamina);
    }

    // percentages
    std::optional<float> health;
    std::optional<float> magicka;
    std::optional<float> stamina;
  };

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("idx", idx)
      .Serialize("data", data);
  }

  std::optional<uint32_t> idx;
  Data data;
};
