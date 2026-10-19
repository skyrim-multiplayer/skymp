#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "script_classes/PapyrusActor.h"

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
  ActorValues actorValues;
  actorValues.healthPercentage = .5f;
  actorValues.magickaPercentage = .5f;
  actorValues.staminaPercentage = .5f;
  actor.SetPercentages(actorValues);

  papyrusActor.RestoreActorValue(actor.ToVarValue(),
                                 { VarValue("HeaLth"), VarValue(100.f) });
  papyrusActor.RestoreActorValue(actor.ToVarValue(),
                                 { VarValue("stamina"), VarValue(-100.f) });
  papyrusActor.RestoreActorValue(actor.ToVarValue(),
                                 { VarValue("MAGICKA"), VarValue(-25.f) });

  MpChangeForm changeForm = actor.GetChangeForm();

  REQUIRE(changeForm.actorValues.healthPercentage == 1.f);
  REQUIRE(changeForm.actorValues.staminaPercentage == 1.f);
  REQUIRE(changeForm.actorValues.magickaPercentage == .75f);

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
  ActorValues actorValues;
  actorValues.healthPercentage = .5f;
  actorValues.magickaPercentage = .5f;
  actorValues.staminaPercentage = .5f;
  actor.SetPercentages(actorValues);

  papyrusActor.DamageActorValue(actor.ToVarValue(),
                                { VarValue("HeaLth"), VarValue(-25.f) });
  papyrusActor.DamageActorValue(actor.ToVarValue(),
                                { VarValue("stamina"), VarValue(-100.f) });
  papyrusActor.DamageActorValue(actor.ToVarValue(),
                                { VarValue("MAGICKA"), VarValue(100.f) });

  MpChangeForm changeForm = actor.GetChangeForm();

  REQUIRE(changeForm.actorValues.healthPercentage == .25f);
  REQUIRE(changeForm.actorValues.staminaPercentage == 0.f);
  REQUIRE(changeForm.actorValues.magickaPercentage == 0.f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("SetActorValue", "[Papyrus][Actor][espm]")
{
  PapyrusActor papyrusActor;

  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& actor = p.worldState.GetFormAt<MpActor>(0xff000000);

  papyrusActor.SetActorValue(actor.ToVarValue(),
                             { VarValue("health"), VarValue(123.f) });
  papyrusActor.SetActorValue(actor.ToVarValue(),
                             { VarValue("StaminaRate"), VarValue(8.f) });
  papyrusActor.SetActorValue(
    actor.ToVarValue(),
    { VarValue("magickaRateMult"), VarValue(137.f) });

  MpChangeForm changeForm = actor.GetChangeForm();

  REQUIRE(changeForm.actorValues.health == 123.f);
  REQUIRE(changeForm.actorValues.staminaRate == 8.f);
  REQUIRE(changeForm.actorValues.magickaRateMult == 137.f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("SetActorValue with unknown name does not crash",
          "[Papyrus][Actor][espm]")
{
  PapyrusActor papyrusActor;

  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& actor = p.worldState.GetFormAt<MpActor>(0xff000000);

  // Unknown AV name should fall back to SpSnippet without crashing
  REQUIRE_NOTHROW(papyrusActor.SetActorValue(
    actor.ToVarValue(),
    { VarValue("unknownActorValue"), VarValue(42.f) }));

  // Known values should be unaffected
  MpChangeForm changeForm = actor.GetChangeForm();
  REQUIRE(changeForm.actorValues.health == Catch::Approx(100.f));

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("RestoreActorValue and DamageActorValue ignore rate AVs",
          "[Papyrus][Actor]")
{
  PapyrusActor papyrusActor;

  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& actor = p.worldState.GetFormAt<MpActor>(0xff000000);

  const float originalHealRate = actor.GetChangeForm().actorValues.healRate;

  // Rate AVs are not percentages; these calls should be no-ops
  REQUIRE_NOTHROW(papyrusActor.RestoreActorValue(
    actor.ToVarValue(), { VarValue("healRate"), VarValue(50.f) }));
  REQUIRE_NOTHROW(papyrusActor.DamageActorValue(
    actor.ToVarValue(), { VarValue("staminaRate"), VarValue(10.f) }));

  MpChangeForm changeForm = actor.GetChangeForm();
  REQUIRE(changeForm.actorValues.healRate == originalHealRate);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("IsDead()", "[Papyrus][Actor]")
{
  PapyrusActor papyrusActor;
  PartOne& partOne = GetPartOne();
  DoConnect(partOne, 0);
  uint32_t formId = partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);
  auto& actor = partOne.worldState.GetFormAt<MpActor>(formId);
  const bool worldActorDead = actor.IsDead();
  auto papyrusActorDead = static_cast<const bool>(
    papyrusActor.IsDead(actor.ToVarValue(), {}).CastToBool());
  REQUIRE(worldActorDead == papyrusActorDead);
  actor.Kill();
  papyrusActorDead = static_cast<bool>(
    papyrusActor.IsDead(actor.ToVarValue(), {}).CastToBool());
  REQUIRE(papyrusActorDead == true);

  partOne.DestroyActor(formId);
  DoDisconnect(partOne, 0);
}
