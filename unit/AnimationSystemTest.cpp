#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "ActionListener.h"

PartOne& GetPartOne();

TEST_CASE("Animations system processes animation events correctly",
          "[AnimationSystem]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);

  auto& actor = p.worldState.GetFormAt<MpActor>(0xff000000);
  AnimationSystem system;
  AnimationData data;
  data.animEventName = "attackStart";
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 1.f);
  system.Process(&actor, data);
  // attackStart consumes 5 points of stamina
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 0.95f);
  data.animEventName = "jumpStart";
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 0.95f);
  system.Process(&actor, data);
  // jumpStart consumes 5 points of stamina
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 0.9f);
}
