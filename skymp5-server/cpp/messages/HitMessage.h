#pragma once
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct HitMessage : public MessageBase<HitMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::OnHit)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("data", data);
  }

  struct Data
  {
    template <class Archive>
    void Serialize(Archive& archive)
    {
      archive.Serialize("taggressor", aggressor)
        .Serialize("isBashAttack", isBashAttack)
        .Serialize("isHitBlocked", isHitBlocked)
        .Serialize("isPowerAttack", isPowerAttack)
        .Serialize("isSneakAttack", isSneakAttack)
        .Serialize("projectile", projectile)
        .Serialize("source", source)
        .Serialize("target", target);
    }

    uint32_t aggressor = 0;
    bool isBashAttack = false;
    bool isHitBlocked = false;
    bool isPowerAttack = false;
    bool isSneakAttack = false;
    uint32_t projectile = 0;
    uint32_t source = 0;
    uint32_t target = 0;
  };

  Data data;
};
