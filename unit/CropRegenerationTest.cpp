#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "CropRegeneration.h"
#include "GetBaseActorValues.h"

PartOne& GetPartOne();

TEST_CASE("CropRegeneration is working correctly", "[CropRegeneration]")
{
  PartOne& p = GetPartOne();

  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  uint32_t baseId = ac.GetBaseId();
  auto look = ac.GetLook();
  uint32_t raceId = look ? look->raceId : 0;
  BaseActorValues baseValues = GetBaseActorValues(baseId, raceId);

  float healthPercentage = 0.0f;
  float magickaPercentage = 1.0f;
  float staminaPercentage = 1.5f;
  float time = 1.0f;

  ac.SetPercentages(0, 0, 1);
  float expectedHealth =
    baseValues.healRate * baseValues.healRateMult * time / 10000.0f;
  float expectedMagicka =
    baseValues.magickaRate * baseValues.magickaRateMult * time / 10000.0f;
  float expectedStamina =
    baseValues.staminaRate * baseValues.staminaRateMult * time / 10000.0f;

  REQUIRE(CropHealthRegeneration(healthPercentage, time, &ac) == 0.0f);
  REQUIRE(CropMagickaRegeneration(magickaPercentage, time, &ac) ==
          expectedMagicka);
  REQUIRE(CropMagickaRegeneration(expectedMagicka, time, &ac) ==
          expectedMagicka);
  REQUIRE(CropStaminaRegeneration(staminaPercentage, time, &ac) == 1.0f);
  ac.SetPercentages(0, 0, 0);
  REQUIRE(CropStaminaRegeneration(expectedStamina, time, &ac) ==
          expectedStamina);

  ac.SetPercentages(0.1f, 0.5f, 0.3f);
  REQUIRE(CropHealthRegeneration(expectedHealth + 0.1f, time, &ac) ==
          expectedHealth + 0.1f);
  REQUIRE(CropMagickaRegeneration(expectedMagicka + 0.5f, time, &ac) ==
          expectedMagicka + 0.5f);
  REQUIRE(CropStaminaRegeneration(expectedStamina + 0.3f, time, &ac) ==
          expectedStamina + 0.3f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
