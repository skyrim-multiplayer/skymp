#pragma once
#include <stdint.h>

struct MpActorValues
{
  float health, 
        magicka,
        stamina, 
        healRate, 
        staminaRate, 
        magickaRate,
        magickaRateMult,
        staminaRateMult,
        healRateMult;
};


float GetBaseActorValues(const uint32_t& baseId, const uint32_t& raceIdOverride)
{
  return 100;
}
