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

PartOne& GetPartOne();

TEST_CASE("Obtaining equipped weapons is working correct. [espm][MpActor]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);

  auto& actor = p.worldState.GetFormAt<MpActor>(0xff000000);
  std::array<std::optional<Inventory::Entry>, 2> wornWeapons =
    actor.GetEquippedWeapon();

  bool condition = !wornWeapons[0].has_value() && !wornWeapons[1].has_value();
  REQUIRE(condition);

  constexpr uint32_t ironDagger = 0x0001397e, warhammer = 0x000d2afe,
                     ironSword = 0x00012eb7;
  bool result = actor.OnEquip(ironDagger);
  wornWeapons = actor.GetEquippedWeapon();

  condition = (wornWeapons[0].has_value() &&
               wornWeapons[0].value().baseId == ironDagger) ||
    (wornWeapons[1].has_value() &&
     wornWeapons[1].value().baseId == ironDagger);
  REQUIRE(condition);

  result = actor.OnEquip(warhammer);
  wornWeapons = actor.GetEquippedWeapon();

  condition = (wornWeapons[0].has_value() &&
               wornWeapons[0].value().baseId == warhammer) ||
    (wornWeapons[1].has_value() && wornWeapons[1].value().baseId == warhammer);
  REQUIRE(condition);

  result = actor.OnEquip(ironDagger);
  result = actor.OnEquip(ironSword);
  wornWeapons = actor.GetEquippedWeapon();

  condition = (wornWeapons[0].has_value() &&
               wornWeapons[0].value().baseId == ironDagger) &&
    (wornWeapons[1].has_value() && wornWeapons[1].value().baseId == ironSword);
  REQUIRE(condition);
}
