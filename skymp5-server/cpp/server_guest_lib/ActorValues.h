#pragma once
#include <tuple>

struct ActorValues
{
  float healthPercentage = 1.f;
  float magickaPercentage = 1.f;
  float staminaPercentage = 1.f;
  float health = 100.f;
  float magicka = 100.f;
  float stamina = 100.f;
  float healRate = 0.7f;
  float magickaRate = 3.0f;
  float staminaRate = 5.0f;
  float healRateMult = 100.f;
  float magickaRateMult = 100.f;
  float staminaRateMult = 100.f;

  auto ToTuple() const
  {
    return std::make_tuple(healthPercentage, magickaPercentage,
                           staminaPercentage, health, magicka, stamina,
                           healRate, magickaRate, staminaRate, healRateMult,
                           magickaRate, staminaRate);
  }

  friend bool operator==(const ActorValues& lhs, const ActorValues& rhs)
  {
    return lhs.ToTuple() == rhs.ToTuple();
  }
  friend bool operator!=(const ActorValues& lhs, const ActorValues& rhs)
  {
    return !(lhs == rhs);
  }
  friend bool operator<(const ActorValues& lhs, const ActorValues& rhs)
  {
    return lhs.ToTuple() < rhs.ToTuple();
  }
  friend bool operator>(const ActorValues& lhs, const ActorValues& rhs)
  {
    return lhs.ToTuple() > rhs.ToTuple();
  }
};
