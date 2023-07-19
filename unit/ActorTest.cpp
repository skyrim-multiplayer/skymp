#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

TEST_CASE(
  "Actor appearance, equipment and isRaceMenuOpen properties should present "
  "in changeForm",
  "[Actor]")
{
  MpActor actor(LocationalData(), FormCallbacks::DoNothing());
  Appearance appearance;
  appearance.raceId = 0x123;
  actor.SetAppearance(&appearance);
  actor.SetEquipment(R"({"inv": {"entries":[]}})");
  actor.SetRaceMenuOpen(true);

  REQUIRE(actor.GetChangeForm().appearanceDump == appearance.ToJson());
  REQUIRE(actor.GetChangeForm().equipmentDump == R"({"inv": {"entries":[]}})");
  REQUIRE(actor.GetChangeForm().isRaceMenuOpen == true);
}

TEST_CASE("Actor should load be able to load appearance, equipment, "
          "isRaceMenuOpen and other properties from changeform",
          "[Actor]")
{
  MpChangeForm changeForm;
  changeForm.isRaceMenuOpen = true;
  changeForm.equipmentDump = R"({"inv": {"entries":[]}})";
  changeForm.appearanceDump = Appearance().ToJson();
  changeForm.recType = MpChangeForm::ACHR;
  changeForm.actorValues.healthPercentage = 1.0f;
  changeForm.actorValues.magickaPercentage = 0.9f;
  changeForm.actorValues.staminaPercentage = 0.0f;
  changeForm.isDead = true;
  changeForm.spawnPoint.cellOrWorldDesc.file = "yay";
  changeForm.spawnPoint.cellOrWorldDesc.shortFormId = 0xDEAD;
  changeForm.spawnPoint.pos = { 1, 2, 3 };
  changeForm.spawnPoint.rot = { 1, 2, 4 };
  changeForm.spawnDelay = 8.0f;
  changeForm.consoleCommandsAllowed = true;

  MpActor actor(LocationalData(), FormCallbacks::DoNothing(), 0xff000000);
  actor.ApplyChangeForm(changeForm);

  REQUIRE(actor.GetChangeForm().isRaceMenuOpen == true);
  REQUIRE(actor.GetChangeForm().equipmentDump == R"({"inv": {"entries":[]}})");
  REQUIRE(actor.GetChangeForm().appearanceDump == Appearance().ToJson());
  REQUIRE(actor.GetChangeForm().actorValues.healthPercentage == 1.0f);
  REQUIRE(actor.GetChangeForm().actorValues.magickaPercentage == 0.9f);
  REQUIRE(actor.GetChangeForm().actorValues.staminaPercentage == 0.0f);
  REQUIRE(actor.GetChangeForm().isDead == true);
  REQUIRE(actor.GetChangeForm().spawnPoint.cellOrWorldDesc.file == "yay");
  REQUIRE(actor.GetChangeForm().spawnPoint.cellOrWorldDesc.shortFormId ==
          0xDEAD);
  REQUIRE(actor.GetChangeForm().spawnPoint.pos == NiPoint3{ 1, 2, 3 });
  REQUIRE(actor.GetChangeForm().spawnPoint.rot == NiPoint3{ 1, 2, 4 });
  REQUIRE(actor.GetChangeForm().spawnDelay == 8.0f);
  REQUIRE(actor.GetChangeForm().consoleCommandsAllowed == true);
}
