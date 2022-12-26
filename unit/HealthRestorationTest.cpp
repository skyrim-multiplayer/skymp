#include "ActionListener.h"
#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <chrono>

PartOne& GetPartOne();

TEST_CASE("Potions restore health", "[Restoration]")
{
  using namespace std::chrono_literals;
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);

  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);
  auto past = std::chrono::steady_clock::now() - 10s;
  ac.SetLastAttributesPercentagesUpdate(past);
  ac.AddItem(0x3EAE3, 1);
  // 0x3EAE3 restores 100 hp
  ac.SetPercentages({ 0.1f, 0.f, 0.f });

  ActionListener::RawMessageData rawMsgData;
  rawMsgData.userId = 0;

  p.GetActionListener().OnEquip(rawMsgData, 0x3EAE3);

  std::chrono::duration<float> timeDuration =
    ac.GetLastAttributesPercentagesUpdate() - std::chrono::steady_clock::now();

  REQUIRE(ac.GetChangeForm().actorValues.healthPercentage == 1.0f);
  REQUIRE(timeDuration.count() < 1.f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
