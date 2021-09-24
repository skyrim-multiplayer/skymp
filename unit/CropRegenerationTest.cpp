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
  REQUIRE_THAT(CropHealthRegeneration(expectedHealth, time, &ac),
               Catch::Matchers::WithinAbs(expectedHealth, 0.000001f));

  REQUIRE_THAT(CropMagickaRegeneration(magickaPercentage, time, &ac),
               Catch::Matchers::WithinAbs(expectedMagicka, 0.000001f));
  REQUIRE_THAT(CropMagickaRegeneration(expectedMagicka, time, &ac),
               Catch::Matchers::WithinAbs(expectedMagicka, 0.000001f));

  REQUIRE(CropStaminaRegeneration(staminaPercentage, time, &ac) == 1.0f);
  ac.SetPercentages(0, 0, 0);
  REQUIRE_THAT(CropStaminaRegeneration(expectedStamina, time, &ac),
               Catch::Matchers::WithinAbs(expectedStamina, 0.000001f));

  REQUIRE_THAT(CropHealthRegeneration(expectedHealth + 0.1f, time, &ac),
               Catch::Matchers::WithinAbs(expectedHealth, 0.000001f));
  REQUIRE_THAT(CropMagickaRegeneration(expectedMagicka * (-1), time, &ac),
               Catch::Matchers::WithinAbs(0.0f, 0.000001f));
  REQUIRE_THAT(
    CropStaminaRegeneration(expectedStamina * 2.0f, time * 2.0f, &ac),
    Catch::Matchers::WithinAbs(expectedStamina * 2.0f, 0.000001f));

  REQUIRE_THAT(CropStaminaRegeneration(expectedStamina, 0, &ac),
               Catch::Matchers::WithinAbs(0.0f, 0.000001f));

  ac.SetPercentages(0.5f, 0.3f, 0.01f);

  REQUIRE_THAT(CropHealthRegeneration(expectedHealth + 0.5f, time, &ac),
               Catch::Matchers::WithinAbs(expectedHealth + 0.5f, 0.000001f));
  REQUIRE_THAT(CropMagickaRegeneration(expectedMagicka + 0.3f, time, &ac),
               Catch::Matchers::WithinAbs(expectedMagicka + 0.3f, 0.000001f));
  REQUIRE_THAT(CropStaminaRegeneration(expectedStamina + 0.01f, time, &ac),
               Catch::Matchers::WithinAbs(expectedStamina + 0.01f, 0.000001f));

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
