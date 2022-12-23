#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "PapyrusActor.h"

PartOne& GetPartOne();

TEST_CASE("RestoreActorValue", "[Papyrus][Actor][espm]")
{
  PapyrusActor papyrusActor;

  using AV = espm::ActorValue;
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& actor = p.worldState.GetFormAt<MpActor>(0xff000000);
  actor.SetPercentages(.5f, .5f, .5f);

  papyrusActor.RestoreActorValue(actor.ToVarValue(),
                                 { VarValue("HeaLth"), VarValue(100.f) });
  papyrusActor.RestoreActorValue(actor.ToVarValue(),
                                 { VarValue("stamina"), VarValue(-100.f) });
  papyrusActor.RestoreActorValue(actor.ToVarValue(),
                                 { VarValue("MAGICKA"), VarValue(-25.f) });

  MpChangeForm changeForm = actor.GetChangeForm();

  REQUIRE(changeForm.healthPercentage == 1.f);
  REQUIRE(changeForm.staminaPercentage == 1.f);
  REQUIRE(changeForm.magickaPercentage == .75f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("DamageActorValue", "[Papyrus][Actor][espm]")
{
  PapyrusActor papyrusActor;

  using AV = espm::ActorValue;
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& actor = p.worldState.GetFormAt<MpActor>(0xff000000);
  actor.SetPercentages(.5f, .5f, .5f);

  papyrusActor.DamageActorValue(actor.ToVarValue(),
                                { VarValue("HeaLth"), VarValue(-25.f) });
  papyrusActor.DamageActorValue(actor.ToVarValue(),
                                { VarValue("stamina"), VarValue(-100.f) });
  papyrusActor.DamageActorValue(actor.ToVarValue(),
                                { VarValue("MAGICKA"), VarValue(100.f) });

  MpChangeForm changeForm = actor.GetChangeForm();

  REQUIRE(changeForm.healthPercentage == .25f);
  REQUIRE(changeForm.staminaPercentage == 0.f);
  REQUIRE(changeForm.magickaPercentage == 0.f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
