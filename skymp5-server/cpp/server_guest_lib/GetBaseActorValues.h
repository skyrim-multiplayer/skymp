#pragma once
#include <stdint.h>

struct BaseActorValues
{
	float health,
		  stamina,
		  magicka,
		  healRate,
		  magickaRate,
		  staminaRate,
		  staminaRateMult,
		  healRateMult,
		  magickaRateMult;
};


BaseActorValues GetBaseActorValues(const uint32_t& baseId, const uint32_t& raceIdOverride)
{
	BaseActorValues baseActorValues;
	baseActorValues.health = 100;
	baseActorValues.stamina = 100;
	baseActorValues.magicka = 100;
	baseActorValues.healRate = 100;
	baseActorValues.staminaRate = 100;
	baseActorValues.magickaRate = 100;
	baseActorValues.healRateMult = 100;
	baseActorValues.magickaRateMult = 100;
	baseActorValues.staminaRateMult = 100;
	return baseActorValues;
}
