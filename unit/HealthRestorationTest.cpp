#include "IActionListener.h"
#include "TestUtils.hpp"
#include <catch2/catch.hpp>

PartOne& GetPartOne();

TEST_CASE("Potions restore health", "[Restoration]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);
  IActionListener::RawMessageData rawMsgData;
  rawMsgData.userId = 0;
  ac.AddItem(0x3EAE3, 1);
  // 0x3EAE3 restores 100 hp
  ac.SetPercentages(0.1f, 0.f, 0.f);
  p.GetActionListener().OnEquip(rawMsgData, 0x3EAE3);

  REQUIRE(ac.GetChangeForm().healthPercentage == 1.0f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
