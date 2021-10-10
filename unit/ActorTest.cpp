#include "TestUtils.hpp"
#include <catch2/catch.hpp>

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

TEST_CASE("Actor should load be able to load appearance, equipment and "
          "isRaceMenuOpen properties from changeform",
          "[Actor]")
{
  MpChangeForm changeForm;
  changeForm.isRaceMenuOpen = true;
  changeForm.equipmentDump = R"({"inv": {"entries":[]}})";
  changeForm.appearanceDump = Appearance().ToJson();
  changeForm.recType = MpChangeForm::ACHR;
  changeForm.healthPercentage = 1.0f;
  changeForm.magickaPercentage = 0.9f;
  changeForm.staminaPercentage = 0.0f;

  MpActor actor(LocationalData(), FormCallbacks::DoNothing(), 0xff000000);
  actor.ApplyChangeForm(changeForm);

  REQUIRE(actor.GetChangeForm().isRaceMenuOpen == true);
  REQUIRE(actor.GetChangeForm().equipmentDump == R"({"inv": {"entries":[]}})");
  REQUIRE(actor.GetChangeForm().appearanceDump == Appearance().ToJson());
  REQUIRE(actor.GetChangeForm().healthPercentage == 1.0f);
  REQUIRE(actor.GetChangeForm().magickaPercentage == 0.9f);
  REQUIRE(actor.GetChangeForm().staminaPercentage == 0.0f);
}
