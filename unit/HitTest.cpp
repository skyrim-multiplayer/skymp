#include "TestUtils.hpp"
#include <catch2/catch.hpp>
#include <chrono>

#include "GetBaseActorValues.h"
#include "Loader.h"
#include "PacketParser.h"

PartOne& GetPartOne();
extern espm::Loader l;

TEST_CASE("OnHit sends a ChangeValues' packet and damage character by "
          "weapon-dependent value",
          "[Hit]")
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

TEST_CASE("OnHit function sends ChangeValues message with coorect percentages",
          "[ChangeValues]")
{
  using namespace std::chrono_literals;

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

  p.Messages().clear();
  p.GetActionListener().OnHit(rawMsgData, hitData);

  REQUIRE(p.Messages().size() == 1);
  nlohmann::json message = p.Messages()[0].j;

  REQUIRE(message["data"]["health"] == 0.96f);
  REQUIRE(message["data"]["magicka"] == 1.0f);
  REQUIRE(message["data"]["stamina"] == 1.0f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("OnHit damage character by race-dependent value", "[Hit]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  // Nord bu default
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  IActionListener::RawMessageData rawMsgData;
  rawMsgData.userId = 0;
  HitData hitData;
  hitData.target = 0x14;
  hitData.aggressor = 0x14;
  hitData.source = 0x1f4; // unarmed attack

  p.Messages().clear();
  p.GetActionListener().OnHit(rawMsgData, hitData);

  REQUIRE(p.Messages().size() == 1);
  auto changeForm = ac.GetChangeForm();
  REQUIRE(changeForm.healthPercentage == 0.96f);
  REQUIRE(changeForm.magickaPercentage == 1.f);
  REQUIRE(changeForm.staminaPercentage == 1.f);

  Appearance appearance;
  appearance.raceId = 0x13745; // KhajiitRace
  ac.SetAppearance(&appearance);
  ac.SetPercentages(1, 1, 1);

  p.GetActionListener().OnHit(rawMsgData, hitData);
  changeForm = ac.GetChangeForm();

  REQUIRE(changeForm.healthPercentage == 0.9f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("OnHit doesn't damage character if it is out of range", "[Hit]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  IActionListener::RawMessageData rawMsgData;
  rawMsgData.userId = 0;

  const uint32_t aggressor = 0xff000000;
  const uint32_t target = 0xff000001;

  p.CreateActor(aggressor, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, aggressor);
  auto& acAggressor = p.worldState.GetFormAt<MpActor>(aggressor);

  p.CreateActor(target, { 0, 0, 0 }, 0, 0x3c);
  auto& acTarget = p.worldState.GetFormAt<MpActor>(target);

  HitData hitData;
  hitData.target = target;
  hitData.aggressor = 0x14;
  hitData.source = 0x0001397E;

  // fCombatDistance global value * reach
  const float awaitedRange = 141.f * 0.7f;
  acTarget.SetPos({ awaitedRange * 1.001f, 0, 0 });
  acTarget.SetPercentages(0.1f, 1, 1);

  p.GetActionListener().OnHit(rawMsgData, hitData);

  auto changeForm = acTarget.GetChangeForm();
  REQUIRE(changeForm.healthPercentage == 0.1f);

  p.DestroyActor(aggressor);
  p.DestroyActor(target);
  DoDisconnect(p, 0);
}
