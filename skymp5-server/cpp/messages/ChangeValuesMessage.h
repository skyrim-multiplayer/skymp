#pragma once
#include "MsgType.h"
#include <optional>
#include <type_traits>

struct ChangeValuesMessage
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
    float health = 0;
    float magicka = 0;
    float stamina = 0;
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
