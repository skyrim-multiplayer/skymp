#pragma once
#include <cstdint>

struct BaseActorValues
{
  float health = 100;
  float stamina = 100;
  float magicka = 100;
  float healRate = 100;
  float staminaRate = 100;
  float magickaRate = 100;
  float healRateMult = 100;
  float staminaRateMult = 100;
  float magickaRateMult = 100;

  std::string VisitBaseActorValues(float attribute)
  {
    std::string visitAttribute = std::to_string(attribute);
    return visitAttribute;
  }
};

inline BaseActorValues GetBaseActorValues(uint32_t baseId,
                                          uint32_t raceIdOverride)
{
  BaseActorValues baseActorValues;
  return baseActorValues;
}
