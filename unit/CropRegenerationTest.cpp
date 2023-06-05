#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <chrono>

#include "CropRegeneration.h"
#include "GetBaseActorValues.h"
#include "libespm/Loader.h"

PartOne& GetPartOne();
extern espm::Loader l;

TEST_CASE("CropRegeneration function is working correctly",
          "[CropRegeneration]")
{
  float secondsAfterLastRegen = 1.0f;
  float attributeRate = 0.7f;
  float attributeRateMult = 100.0f;
  float oldAttributeValue = 0.6f;

  float validAttributeValueRegeneration = attributeRate / 100.0f *
    attributeRateMult / 100.0f * secondsAfterLastRegen;

  float newAttributeValue =
    oldAttributeValue + validAttributeValueRegeneration;

  REQUIRE(CropRegeneration(newAttributeValue, 1.0f, 0.7f, 100.0f, 0.6f) ==
          newAttributeValue);
}

TEST_CASE(
  "CropRegeneration returns oldAttributeValue if regeneration is not positive",
  "[CropRegeneration]")
{
  float oldAttributeValue = 0.6f;

  float newAttributeValue = oldAttributeValue + 0.007f;

  REQUIRE(CropRegeneration(newAttributeValue, 1.0f, 0.7f, -100.0f,
                           oldAttributeValue) == oldAttributeValue);
}

TEST_CASE(
  "CropRegeneration returns 1 if regeneration is enough to restore attribute",
  "[CropRegeneration]")
{
  REQUIRE(CropRegeneration(1.0f, 1.0f, 5.0f, 100.0f, 0.97f) == 1.0f);
}

TEST_CASE("CropRegeneration returns 1 if newAttributeValue is more then 1 "
          "when oldAttributeValue = 1",
          "[CropRegeneration]")
{
  REQUIRE(CropRegeneration(1.05f, 1.0f, 5.0f, 100.0f, 1.0f) == 1.0f);
}

TEST_CASE("CropRegeneration returns the correct value if newAttributeValue is "
          "too large but oldAttributeValue is equal to zero",
          "[CropRegeneration]")
{
  REQUIRE(CropRegeneration(1.0f, 1.0f, 5.0f, 100.0f, 0.0f) == 0.05f);
}

TEST_CASE("CropPeriodAfterLastRegen returns 0 if period < 0",
          "[CropRegeneration]")
{
  REQUIRE(CropPeriodAfterLastRegen(-1.0f) == 0.0f);
}

TEST_CASE(
  "CropPeriodAfterLastRegen returns defaultPeriod if period > maxValidPeriod",
  "[CropRegeneration]")
{
  float defaultPeriod = 1.0f;
  float maxValidPeriod = 2.0f;
  REQUIRE(CropPeriodAfterLastRegen(2.5f, maxValidPeriod, defaultPeriod) ==
          1.0f);
}

TEST_CASE("CropPeriodAfterLastRegen returns correct value if period is in "
          "0...maxValidPeriod interval",
          "[CropRegeneration]")
{
  float defaultPeriod = 1.0f;
  float maxValidPeriod = 2.0f;
  REQUIRE(CropPeriodAfterLastRegen(1.3f, maxValidPeriod, defaultPeriod) ==
          1.3f);
}

TEST_CASE("CropHealthRegeneration, CropMagickaRegeneration and "
          "CropStaminaRegeneration are working correctly, regeneration is not "
          "too fast",
          "[CropRegeneration]")
{

  using namespace std::chrono_literals;

  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  uint32_t baseId = ac.GetBaseId();
  auto appearance = ac.GetAppearance();
  uint32_t raceId = appearance ? appearance->raceId : 0;
  BaseActorValues baseValues =
    GetBaseActorValues(&p.worldState, baseId, raceId);

  ac.SetPercentages({ 0.0f, 0.0f, 0.0f });

  auto past = std::chrono::steady_clock::now();
  auto now = past + 1s;
  ac.SetLastAttributesPercentagesUpdate(past);
  std::chrono::duration<float> timeDuration = now - past;
  float time = timeDuration.count();

  float expectedHealth =
    baseValues.healRate * baseValues.healRateMult * time / 10000.0f;
  float expectedMagicka =
    baseValues.magickaRate * baseValues.magickaRateMult * time / 10000.0f;
  float expectedStamina =
    baseValues.staminaRate * baseValues.staminaRateMult * time / 10000.0f;

  REQUIRE_THAT(CropHealthRegeneration(1.0f, time, &ac),
               Catch::Matchers::WithinAbs(expectedHealth, 0.000001f));
  REQUIRE_THAT(CropMagickaRegeneration(1.0f, time, &ac),
               Catch::Matchers::WithinAbs(expectedMagicka, 0.000001f));
  REQUIRE_THAT(CropStaminaRegeneration(1.0f, time, &ac),
               Catch::Matchers::WithinAbs(expectedStamina, 0.000001f));

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
