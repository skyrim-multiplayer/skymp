#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <chrono>

#include "GetBaseActorValues.h"
#include "HitData.h"
#include "PacketParser.h"
#include "libespm/Loader.h"

PartOne& GetPartOne();
extern espm::Loader l;
using namespace std::chrono_literals;

TEST_CASE("OnHit damages target actor based on damage formula", "[Hit]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  ActionListener::RawMessageData rawMsgData;
  rawMsgData.userId = 0;
  HitData hitData;
  hitData.target = 0x14;
  hitData.aggressor = 0x14;
  hitData.source = 0x0001397E; // iron dagger 4 damage, id = 80254
  ac.AddItem(hitData.source, 1);
  ac.SetEquipment(R"(
    {
      "inv": {
        "entries": [
          {
            "baseId": 80254,
            "count": 1,
            "worn": true
          }
        ]
      }
    }
  )");

  auto past = std::chrono::steady_clock::now() - 10s;
  ac.SetLastHitTime(past);
  p.Messages().clear();
  p.GetActionListener().OnHit(rawMsgData, hitData);

  REQUIRE(p.Messages().size() == 1);
  auto changeForm = ac.GetChangeForm();
  REQUIRE(changeForm.actorValues.healthPercentage == 0.75f);
  REQUIRE(changeForm.actorValues.magickaPercentage == 1.f);
  REQUIRE(changeForm.actorValues.staminaPercentage == 1.f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("OnHit function sends ChangeValues message with coorect percentages",
          "[TES5DamageFormula]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);
  ac.SetEquipment(R"({"inv": {"entries": []}})");

  ActionListener::RawMessageData rawMsgData;
  rawMsgData.userId = 0;
  HitData hitData;
  hitData.target = 0x14;
  hitData.aggressor = 0x14;
  hitData.source = 0x0001397E; // iron dagger 4 damage
  ac.AddItem(hitData.source, 1);
  ac.SetEquipment(R"(
    {
      "inv": {
        "entries": [
          {
            "baseId": 80254,
            "count": 1,
            "worn": true
          }
        ]
      }
    }
  )");

  p.Messages().clear();
  auto past = std::chrono::steady_clock::now() - 4s;
  ac.SetLastHitTime(past);
  p.GetActionListener().OnHit(rawMsgData, hitData);

  REQUIRE(p.Messages().size() == 1);
  nlohmann::json message = p.Messages()[0].j;

  REQUIRE(message["data"]["health"] == 0.75f);
  REQUIRE(message["data"]["magicka"] == 1.0f);
  REQUIRE(message["data"]["stamina"] == 1.0f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("OnHit doesn't damage character if it is out of range", "[Hit]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  ActionListener::RawMessageData rawMsgData;
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

  int16_t face =
    espm::GetData<espm::NPC_>(acAggressor.GetBaseId(), &p.worldState)
      .objectBounds.pos2[1];
  int16_t targetSide =
    espm::GetData<espm::NPC_>(acTarget.GetBaseId(), &p.worldState)
      .objectBounds.pos2[1];

  // fCombatDistance global value * reach
  const float awaitedRange = 141.f * 0.7f + face + targetSide;
  acTarget.SetPos({ awaitedRange * 1.001f, 0, 0 });
  acTarget.SetAngle({ 0.f, 0.f, 180.f });
  ActorValues actorValues;
  actorValues.healthPercentage = 0.1f;
  actorValues.magickaPercentage = 1.f;
  actorValues.staminaPercentage = 1.f;
  acTarget.SetPercentages(actorValues);

  auto past = std::chrono::steady_clock::now() - 2s;
  acTarget.SetLastHitTime(past);
  acAggressor.SetLastHitTime(past);
  p.GetActionListener().OnHit(rawMsgData, hitData);

  auto changeForm = acTarget.GetChangeForm();
  REQUIRE(changeForm.actorValues.healthPercentage == 0.1f);

  p.DestroyActor(aggressor);
  p.DestroyActor(target);
  DoDisconnect(p, 0);
}

TEST_CASE("Dead actors can't attack", "[Hit]")
{
  PartOne& p = GetPartOne();
  ActionListener::RawMessageData rawMsgData;

  const uint32_t aggressor = 0xff000000;
  const uint32_t target = 0xff000001;

  p.CreateActor(aggressor, { 0, 0, 0 }, 0, 0x3c);
  p.CreateActor(target, { 0, 0, 0 }, 0, 0x3c);

  DoConnect(p, 0);
  p.SetUserActor(0, aggressor);
  rawMsgData.userId = 0;

  HitData hitData;
  hitData.target = target;
  hitData.aggressor = 0x14;
  hitData.source = 0x0001397E;

  auto& acTarget = p.worldState.GetFormAt<MpActor>(target);
  ActorValues actorValues;
  actorValues.healthPercentage = 0.2f;
  actorValues.magickaPercentage = 1.f;
  actorValues.staminaPercentage = 1.f;
  acTarget.SetPercentages(actorValues);

  auto& acAggressor = p.worldState.GetFormAt<MpActor>(aggressor);
  acAggressor.Kill();
  REQUIRE(acAggressor.IsDead() == true);

  p.GetActionListener().OnHit(rawMsgData, hitData);

  REQUIRE(acTarget.GetChangeForm().actorValues.healthPercentage == 0.2f);

  p.DestroyActor(aggressor);
  p.DestroyActor(target);
  DoDisconnect(p, 0);
}

TEST_CASE("checking weapon cooldown", "[Hit]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);

  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  ActorValues actorValues;
  actorValues.healthPercentage = 1.f;
  actorValues.magickaPercentage = 1.f;
  actorValues.staminaPercentage = 1.f;
  ac.SetPercentages(actorValues);

  ActionListener::RawMessageData msgData;
  msgData.userId = 0;
  HitData hitData;
  hitData.target = 0x14;
  hitData.aggressor = 0x14;
  hitData.source = 0x0001397E;
  ac.AddItem(hitData.source, 1);
  ac.SetEquipment(R"(
    {
      "inv": {
        "entries": [
          {
            "baseId": 80254,
            "count": 1,
            "worn": true
          }
        ]
      }
    }
  )");

  auto past = std::chrono::steady_clock::now() - 300ms;

  ac.SetLastHitTime(past);
  p.Messages().clear();
  p.GetActionListener().OnHit(msgData, hitData);

  auto current = ac.GetLastHitTime();
  std::chrono::duration<float> duration = current - past;
  float passedTime = duration.count();
  float daggerSpeed = 1.3f;

  REQUIRE(passedTime <= 1.1 * (1 / daggerSpeed));
  REQUIRE(p.Messages().size() == 0);

  past = std::chrono::steady_clock::now() - 3s;
  ac.SetLastHitTime(past);
  p.Messages().clear();
  p.GetActionListener().OnHit(msgData, hitData);
  current = ac.GetLastHitTime();
  duration = current - past;
  passedTime = duration.count();

  REQUIRE(passedTime >= 1.1 * (1 / daggerSpeed));
  REQUIRE(p.Messages().size() == 1);
  nlohmann::json message = p.Messages()[0].j;
  uint64_t msgType = 16; // OnHit sends ChangeValues message type
  REQUIRE(message["t"] == msgType);
  REQUIRE(message["data"]["health"] == 0.75f);
  REQUIRE(message["data"]["magicka"] == 1.f);
  REQUIRE(message["data"]["stamina"] == 1.f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
