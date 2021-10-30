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

TEST_CASE("Formula takes weapon damage into account",
          "[TES5DamageFormula]")
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

  TES5DamageFormula formula(ac, ac, hitData);
  REQUIRE(formula.CalculateDamage() == 4.0f);

  // vvv temporary stuff to ensure I didn't break anything

  p.SetDamageFormulaFactory([](const MpActor& aggressor, const MpActor& target, const HitData& hitData) {
    return std::make_unique<TES5DamageFormula>(aggressor, target, hitData);
  });

  auto past = std::chrono::steady_clock::now() - 10s;
  ac.SetLastHitTime(past);
  p.Messages().clear();
  p.GetActionListener().OnHit(rawMsgData, hitData);

  REQUIRE(p.Messages().size() == 1);
  auto changeForm = ac.GetChangeForm();
  REQUIRE(changeForm.healthPercentage == 0.96f);
  REQUIRE(changeForm.magickaPercentage == 1.f);
  REQUIRE(changeForm.staminaPercentage == 1.f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("Damage is reduced based on target's armor",
          "[TES5DamageFormula]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  // 77382 = 0x12e46: Iron Gauntlets, rating = 10
  // 77387 = 0x12e4b: Iron Boots, rating = 25
  // 77389 = 0x12e4d: Iron Helmet, rating = 15
  // Total rating for worn armor: 10 + 25 = 35
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

  IActionListener::RawMessageData rawMsgData;
  rawMsgData.userId = 0;
  HitData hitData;
  hitData.target = 0x14;
  hitData.aggressor = 0x14;
  hitData.source = 0x0001397E; // iron dagger 4 damage

  TES5DamageFormula formula(ac, ac, hitData);
  REQUIRE(formula.CalculateDamage() == 0.8f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("Formula is race-dependent for unarmed attack", "[TES5DamageFormula]")
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
    TES5DamageFormula formula(ac, ac, hitData);
    REQUIRE(formula.CalculateDamage() == 4.0f);
  }

  // vvv temporary stuff to ensure I didn't break anything

  p.SetDamageFormulaFactory([](const MpActor& aggressor, const MpActor& target, const HitData& hitData) {
    return std::make_unique<TES5DamageFormula>(aggressor, target, hitData);
  });

  p.Messages().clear();
  auto past = std::chrono::steady_clock::now() - 2s;
  ac.SetLastHitTime(past);
  p.GetActionListener().OnHit(rawMsgData, hitData);

  REQUIRE(p.Messages().size() == 1);
  auto changeForm = ac.GetChangeForm();
  REQUIRE(changeForm.healthPercentage == 0.96f);
  REQUIRE(changeForm.magickaPercentage == 1.f);
  REQUIRE(changeForm.staminaPercentage == 1.f);

  // ^^^

  Appearance appearance;
  appearance.raceId = 0x13745; // KhajiitRace
  ac.SetAppearance(&appearance);
  ac.SetPercentages(1, 1, 1);

  {
    TES5DamageFormula formula(ac, ac, hitData);
    REQUIRE(formula.CalculateDamage() == 10.0f);
  }

  past = std::chrono::steady_clock::now() - 2s;
  ac.SetLastHitTime(past);
  p.GetActionListener().OnHit(rawMsgData, hitData);
  changeForm = ac.GetChangeForm();

  REQUIRE(changeForm.healthPercentage == 0.9f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
