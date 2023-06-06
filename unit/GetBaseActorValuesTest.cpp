#include "TestUtils.hpp"
#include "libespm/GroupUtils.h"
#include "libespm/Loader.h"
#include <catch2/catch_all.hpp>

#include "GetBaseActorValues.h"

extern espm::Loader l;
PartOne& GetPartOne();

TEST_CASE("GetBaseActorValues works correctly", "[GetBaseActorValues]")
{
  // Ri'saad is a Khajiit roving merchant from caravan.
  const uint32_t kRisaadFormId = 0x0001B1DB;

  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(kRisaadFormId, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, kRisaadFormId);
  auto& ac = p.worldState.GetFormAt<MpActor>(kRisaadFormId);

  uint32_t baseId = ac.GetBaseId();
  auto appearance = ac.GetAppearance();
  uint32_t raceId = appearance ? appearance->raceId : 0;
  BaseActorValues baseValues =
    GetBaseActorValues(&p.worldState, baseId, raceId);

  REQUIRE(baseValues.health == 100.f);
  REQUIRE(baseValues.stamina == 100.f);
  REQUIRE(baseValues.magicka == 100.f);
  REQUIRE(baseValues.healRate == 0.7f);
  REQUIRE(baseValues.staminaRate == 5.f);
  REQUIRE(baseValues.magickaRate == 3.f);
}
