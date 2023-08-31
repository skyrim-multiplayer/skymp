#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "ActionListener.h"

PartOne& GetPartOne();

TEST_CASE("Animations system processes animation events correctly",
          "[AnimationSystem]")
{
  PartOne& p = GetPartOne();
  p.animationSystem.Init(&p.worldState);

  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);

  auto& actor = p.worldState.GetFormAt<MpActor>(0xff000000);
  AnimationData data;

  // Normally, stamina is not consumed when playing animations
  data.animEventName = "attackStart";
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 1.f);
  p.animationSystem.Process(&actor, data);
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 1.f);
  data.animEventName = "JumpStandingStart";
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 1.f);
  p.animationSystem.Process(&actor, data);
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 1.f);

  // Sweetpie
  p.worldState.espmFiles.push_back("SweetPie.esp");
  p.animationSystem.Init(&p.worldState);

  data.animEventName = "attackStart";
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 1.f);
  p.animationSystem.Process(&actor, data);
  // attackStart consumes 7 points of stamina
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 0.93f);
  data.animEventName = "JumpStandingStart";
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 0.93f);
  p.animationSystem.Process(&actor, data);
  // JumpStandingStart consumes 10 points of stamina
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 0.83f);
}
