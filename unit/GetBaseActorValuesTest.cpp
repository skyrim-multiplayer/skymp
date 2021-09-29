#include "TestUtils.hpp"
#include <GroupUtils.h>
#include <Loader.h>
#include <catch2/catch.hpp>

#include "GetBaseActorValues.h"

extern espm::Loader l;
PartOne& GetPartOne();

TEST_CASE("GetBaseActorValues works correctly", "[GetBaseActorValues]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(
    0x0001B1DB, { 0, 0, 0 }, 0,
    0x3c); // Ri'saad's id. (Roving merchant from Khajiit's caravan.)
  p.SetUserActor(0, 0x0001B1DB);
  auto& ac = p.worldState.GetFormAt<MpActor>(0x0001B1DB);

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