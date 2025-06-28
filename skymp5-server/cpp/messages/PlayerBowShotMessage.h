#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct PlayerBowShotMessage : public MessageBase<PlayerBowShotMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::PlayerBowShot)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("weaponId", weaponId)
      .Serialize("ammoId", ammoId)
      .Serialize("power", power)
      .Serialize("isSunGazing", isSunGazing);
  }

  uint32_t weaponId = 0;
  uint32_t ammoId = 0;
  float power = 0.f;
  bool isSunGazing = false;
};
