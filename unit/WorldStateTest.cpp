#include "WorldState.h"
#include "FormCallbacks.h"
#include "MpActor.h"
#include "MpForm.h"
#include "MsgType.h"
#include "PartOne.h"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>

using Catch::Matchers::ContainsSubstring;

TEST_CASE("AddForm failures", "[WorldState]")
{
  WorldState worldState;
  worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0xff000000);
  REQUIRE_THROWS_WITH(
    worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0xff000000),
    ContainsSubstring("Form with id ff000000 already exists"));
}

TEST_CASE("DestroyForm failures", "[WorldState]")
{
  WorldState worldState;
  REQUIRE_THROWS_WITH(
    worldState.DestroyForm(0x12345678),
    ContainsSubstring("Form with id 12345678 doesn't exist"));

  worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0x12345678);
  REQUIRE_THROWS_WITH(
    worldState.DestroyForm<MpActor>(0x12345678),
    ContainsSubstring("Expected form 12345678 to be Actor, but got Form"));
}

TEST_CASE("Load ChangeForm of created Actor", "[WorldState]")
{
  WorldState worldState;
  worldState.espmFiles = { "Morrowind.esm", "Tribunal.esm" };

  MpChangeForm changeForm;
  changeForm.recType = MpChangeForm::ACHR;
  changeForm.position = { 1, 2, 3 };
  changeForm.worldOrCellDesc = FormDesc::Tamriel();
  changeForm.baseDesc = { 0xabcd, "Tribunal.esm" };

  worldState.LoadChangeForm(changeForm, FormCallbacks::DoNothing());

  auto& refr = worldState.GetFormAt<MpActor>(0xff000000);
  REQUIRE(refr.GetFormId() == 0xff000000);
  REQUIRE(refr.GetChangeForm().formDesc.ToString() == "0");
  REQUIRE(refr.GetPos() == NiPoint3{ 1, 2, 3 });
  REQUIRE(refr.GetCellOrWorld() == FormDesc::Tamriel());
  REQUIRE(refr.GetBaseId() == 0x0100abcd);
}

TEST_CASE("Load ChangeForm of created Actor with isDisabled=true",
          "[WorldState]")
{
  WorldState worldState;
  worldState.espmFiles = { "Morrowind.esm", "Tribunal.esm" };

  MpChangeForm changeForm;
  changeForm.recType = MpChangeForm::ACHR;
  changeForm.worldOrCellDesc = FormDesc::FromString("dead:Morrowind.esm");
  changeForm.baseDesc = { 0xabcd, "Tribunal.esm" };
  changeForm.isDisabled = true;

  worldState.LoadChangeForm(changeForm, FormCallbacks::DoNothing());

  auto& refr = worldState.GetFormAt<MpActor>(0xff000000);
  REQUIRE(refr.IsDisabled());

  // Disabled actors should not pollute grids during load process
  REQUIRE(worldState.GetGrids().count(0xdead) == 0);
}

TEST_CASE("Load ChangeForm of created Actor with profileId", "[WorldState]")
{
  WorldState worldState;
  worldState.espmFiles = { "Morrowind.esm", "Tribunal.esm" };

  MpChangeForm changeForm;
  changeForm.recType = MpChangeForm::ACHR;
  changeForm.worldOrCellDesc = FormDesc::FromString("dead:Morrowind.esm");
  changeForm.baseDesc = { 0xabcd, "Tribunal.esm" };
  changeForm.isDisabled = true;
  changeForm.profileId = 100;

  REQUIRE(worldState.GetActorsByProfileId(100).empty());
  worldState.LoadChangeForm(changeForm, FormCallbacks::DoNothing());

  REQUIRE(worldState.GetGrids().count(0xdead) == 0);
  REQUIRE(worldState.GetActorsByProfileId(100) ==
          std::set<uint32_t>({ 0xff000000 }));
}

TEST_CASE("Load ChangeForm of modified object", "[WorldState]")
{
  WorldState worldState;
  worldState.espmFiles = { "Skyrim.esm" };

  MpChangeForm changeForm;
  changeForm.formDesc = { 0xeeee, "Skyrim.esm" };
  changeForm.position = { 1, 2, 3 };
  changeForm.worldOrCellDesc = FormDesc::Tamriel();
  changeForm.baseDesc = { 0xabcd, "Skyrim.esm" };

  auto newRefr = new MpObjectReference(
    LocationalData(), FormCallbacks::DoNothing(), 0x0000abcd, "STAT");
  worldState.AddForm(std::unique_ptr<MpObjectReference>(newRefr), 0xeeee);

  worldState.LoadChangeForm(changeForm, FormCallbacks::DoNothing());
  auto& refr = worldState.GetFormAt<MpObjectReference>(0xeeee);
  REQUIRE(refr.GetFormId() == 0xeeee);
  REQUIRE(refr.GetChangeForm().formDesc.ToString() == "eeee:Skyrim.esm");
  REQUIRE(refr.GetPos() == NiPoint3{ 1, 2, 3 });
  REQUIRE(refr.GetCellOrWorld() == FormDesc::Tamriel());
  REQUIRE(refr.GetBaseId() == 0x0000abcd);
  REQUIRE(refr.Type() == std::string("ObjectReference"));
  REQUIRE(&refr == newRefr);
}

TEST_CASE("Load ChangeForm of modified object with changed baseType",
          "[WorldState]")
{
  WorldState worldState;
  worldState.espmFiles = { "Skyrim.esm" };
  auto newRefr = new MpObjectReference(
    LocationalData(), FormCallbacks::DoNothing(), 0x0000ded0, "STAT");
  worldState.AddForm(std::unique_ptr<MpObjectReference>(newRefr), 0xeeee);

  MpChangeForm changeForm;
  changeForm.formDesc = { 0xeeee, "Skyrim.esm" };
  changeForm.baseDesc = { 0xabcd, "Skyrim.esm" };

  REQUIRE_THROWS_WITH(
    worldState.LoadChangeForm(changeForm, FormCallbacks::DoNothing()),
    ContainsSubstring("Anomally, baseId should never change (ded0 => abcd)"));
}

extern PartOne& GetPartOne();

TEST_CASE("Loads VirtualMachine with all scripts", "[WorldState]")
{
  auto& p = GetPartOne();
  p.worldState.GetPapyrusVm();
}
