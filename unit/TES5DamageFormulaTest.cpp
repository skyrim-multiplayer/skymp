#include "TestUtils.hpp"
#include <catch2/catch.hpp>
#include <chrono>

#include "GetBaseActorValues.h"
#include "Loader.h"
#include "PacketParser.h"
#include "formulas/TES5DamageFormula.h"

PartOne& GetPartOne();
extern espm::Loader l;
using namespace std::chrono_literals;

TEST_CASE("Formula takes weapon damage into account", "[TES5DamageFormula]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  ac.SetEquipment(R"({"inv": {"entries": []}})");

  IActionListener::RawMessageData rawMsgData;
  rawMsgData.userId = 0;
  HitData hitData;
  hitData.target = 0x14;
  hitData.aggressor = 0x14;
  hitData.source = 0x0001397E; // iron dagger 4 damage

  TES5DamageFormula formula{};
  REQUIRE(formula.CalculateDamage(ac, ac, hitData) == 4.0f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("Damage is reduced based on target's armor", "[TES5DamageFormula]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  IActionListener::RawMessageData rawMsgData;
  rawMsgData.userId = 0;
  HitData hitData;
  hitData.target = 0x14;
  hitData.aggressor = 0x14;
  hitData.source = 0x0001397E; // iron dagger 4 damage

  // 77382 = 0x12e46: Iron Gauntlets, rating = 10
  // 77387 = 0x12e4b: Iron Boots, rating = 10
  // 77389 = 0x12e4d: Iron Helmet, rating = 15
  // Total rating for worn armor: 10 + 10 = 20
  ac.SetEquipment(R"(
    {
      "inv": {
        "entries": [
          {
            "baseId": 77382,
            "count": 1,
            "worn": true
          },
          {
            "baseId": 77387,
            "count": 1,
            "worn": true
          },
          {
            "baseId": 77389,
            "count": 1,
            "worn": false
          }
        ]
      }
    }
  )");

  TES5DamageFormula formula{};
  // 4 / (20 * .12 + 1)
  REQUIRE(formula.CalculateDamage(ac, ac, hitData) == 1.1764705882352942f);

  // Total rating for worn armor: 10 + 10 + 15 = 35
  ac.SetEquipment(R"(
    {
      "inv": {
        "entries": [
          {
            "baseId": 77382,
            "count": 1,
            "worn": true
          },
          {
            "baseId": 77387,
            "count": 1,
            "worn": true
          },
          {
            "baseId": 77389,
            "count": 1,
            "worn": true
          }
        ]
      }
    }
  )");

  // 4 / (35 * .12 + 1) = 0.7692307692307692
  // But minReceivedDamage = 4 * 20% = 0.8
  REQUIRE(formula.CalculateDamage(ac, ac, hitData) == 0.8f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("Formula is race-dependent for unarmed attack",
          "[TES5DamageFormula]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  // Nord bu default
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);
  ac.SetEquipment(R"({"inv": {"entries": []}})");

  IActionListener::RawMessageData rawMsgData;
  rawMsgData.userId = 0;
  HitData hitData;
  hitData.target = 0x14;
  hitData.aggressor = 0x14;
  hitData.source = 0x1f4; // unarmed attack

  {
    TES5DamageFormula formula{};
    REQUIRE(formula.CalculateDamage(ac, ac, hitData) == 4.0f);
  }

  Appearance appearance;
  appearance.raceId = 0x13745; // KhajiitRace
  ac.SetAppearance(&appearance);
  ac.SetPercentages(1, 1, 1);

  {
    TES5DamageFormula formula{};
    REQUIRE(formula.CalculateDamage(ac, ac, hitData) == 10.0f);
  }

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
