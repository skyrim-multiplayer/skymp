#include "TestUtils.hpp"
#include <GroupUtils.h>
#include <Loader.h>
#include <catch2/catch.hpp>

#include "GetBaseActorValues.h"

extern espm::Loader l;
PartOne& GetPartOne();

TEST_CASE("GetBaseActorValues works correctly", "[GetBaseActorValues]")
{
  // Ri'saad is a Khajiit roving merchant from caravan.
  const uint32_t g_risaadFormId = 0x0001B1DB;

  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(g_risaadFormId, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, g_risaadFormId);
  auto& ac = p.worldState.GetFormAt<MpActor>(g_risaadFormId);

  uint32_t baseId = ac.GetBaseId();
  auto look = ac.GetLook();
  uint32_t raceId = look ? look->raceId : 0;
  BaseActorValues baseValues = GetBaseActorValues(l, baseId, raceId);

  REQUIRE(baseValues.health == 50.f);
  REQUIRE(baseValues.stamina == 50.f);
  REQUIRE(baseValues.magicka == 50.f);
  REQUIRE(baseValues.healRate == 0.7f);
  REQUIRE(baseValues.staminaRate == 5.f);
  REQUIRE(baseValues.magickaRate == 3.f);
}
