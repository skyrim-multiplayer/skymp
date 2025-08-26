#include "WorldState.h"
#include "FormCallbacks.h"
#include "GridService.h"
#include "MpActor.h"
#include "MpForm.h"
#include "MsgType.h"
#include "PartOne.h"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>

extern PartOne& GetPartOne();

using Catch::Matchers::ContainsSubstring;

TEST_CASE("AddForm failures", "[WorldState]")
{
  auto& partOne = GetPartOne();
  auto& worldState = partOne.worldState;
  worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0xff000000);
  REQUIRE_THROWS_WITH(
    worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0xff000000),
    ContainsSubstring("Form with id ff000000 already exists"));
}

TEST_CASE("DestroyForm failures", "[WorldState]")
{
  auto& partOne = GetPartOne();
  auto& worldState = partOne.worldState;
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
  auto& partOne = GetPartOne();
  auto& worldState = partOne.worldState;

  MpChangeForm changeForm;
  changeForm.recType = MpChangeForm::ACHR;
  changeForm.position = { 1, 2, 3 };
  changeForm.worldOrCellDesc = FormDesc::Tamriel();
  changeForm.baseDesc = { 0x7, "Skyrim.esm" };

  worldState.LoadChangeForm(changeForm, FormCallbacks::DoNothing());

  auto& refr = worldState.GetFormAt<MpActor>(0xff000000);
  REQUIRE(refr.GetFormId() == 0xff000000);
  REQUIRE(refr.GetChangeForm().formDesc.ToString() == "0");
  REQUIRE(refr.GetPos() == NiPoint3{ 1, 2, 3 });
  REQUIRE(refr.GetCellOrWorld() == FormDesc::Tamriel());
  REQUIRE(refr.GetBaseId() == 0x7);
}

TEST_CASE("Load ChangeForm of created Actor with isDisabled=true",
          "[WorldState]")
{
  auto& partOne = GetPartOne();
  auto& worldState = partOne.worldState;

  MpChangeForm changeForm;
  changeForm.recType = MpChangeForm::ACHR;
  changeForm.worldOrCellDesc = FormDesc::FromString("3c:Skyrim.esm");
  changeForm.baseDesc = { 0x00000007, "Skyrim.esm" };
  changeForm.isDisabled = true;

  worldState.LoadChangeForm(changeForm, FormCallbacks::DoNothing());

  auto& refr = worldState.GetFormAt<MpActor>(0xff000000);
  REQUIRE(refr.IsDisabled());

  // Disabled actors should not pollute grids during load process
  REQUIRE(worldState.GetGridService().GetGrids().count(0x3c) == 0);
}

TEST_CASE("Load ChangeForm of created Actor with profileId", "[WorldState]")
{
  auto& partOne = GetPartOne();
  auto& worldState = partOne.worldState;

  MpChangeForm changeForm;
  changeForm.recType = MpChangeForm::ACHR;
  changeForm.worldOrCellDesc = FormDesc::FromString("3c:Skyrim.esm");
  changeForm.baseDesc = { 0x7, "Skyrim.esm" };
  changeForm.isDisabled = true;
  changeForm.profileId = 100;
  changeForm.formDesc =
    FormDesc::FromFormId(0xff000000, partOne.worldState.espmFiles);

  REQUIRE(worldState.GetActorsByProfileId(100).empty());
  worldState.LoadChangeForm(changeForm, FormCallbacks::DoNothing());

  REQUIRE(worldState.GetGridService().GetGrids().count(0x3c) == 0);
  REQUIRE(worldState.GetActorsByProfileId(100) ==
          std::set<uint32_t>({ 0xff000000 }));
}

TEST_CASE("Load ChangeForm of modified object", "[WorldState]")
{
  constexpr uint32_t kBarrelInWhiterun = 0x4cc2d;

  auto& partOne = GetPartOne();
  auto& worldState = partOne.worldState;

  MpChangeForm changeForm;
  changeForm.formDesc = { kBarrelInWhiterun, "Skyrim.esm" };
  changeForm.position = { 1, 2, 3 };
  changeForm.worldOrCellDesc = FormDesc::Tamriel();
  changeForm.baseDesc = { 0xabcd, "Skyrim.esm" };

  auto newRefr = new MpObjectReference(
    LocationalData(), FormCallbacks::DoNothing(), 0x0000abcd, "STAT");
  worldState.AddForm(std::unique_ptr<MpObjectReference>(newRefr),
                     kBarrelInWhiterun);

  worldState.LoadChangeForm(changeForm, FormCallbacks::DoNothing());
  auto& refr = worldState.GetFormAt<MpObjectReference>(kBarrelInWhiterun);
  REQUIRE(refr.GetFormId() == kBarrelInWhiterun);
  REQUIRE(refr.GetChangeForm().formDesc.ToString() ==
          FormDesc({ kBarrelInWhiterun, "Skyrim.esm" }).ToString());
  REQUIRE(refr.GetPos() == NiPoint3{ 1, 2, 3 });
  REQUIRE(refr.GetCellOrWorld() == FormDesc::Tamriel());
  REQUIRE(refr.GetBaseId() == 0x0000abcd);
  REQUIRE(refr.Type() == std::string("ObjectReference"));
  REQUIRE(&refr == newRefr);
}

TEST_CASE("Load ChangeForm of modified object with changed baseType",
          "[WorldState]")
{
  constexpr auto kBarrelInWhiterun = 0x0004cc2d;
  constexpr auto kBarrelInWhiterunBase = 0x00000845;
  constexpr auto kBarrelInWhiterunBaseNew = 0x00000007;

  auto& partOne = GetPartOne();
  auto& worldState = partOne.worldState;

  MpChangeForm changeForm;
  changeForm.formDesc = { kBarrelInWhiterun, "Skyrim.esm" };
  changeForm.baseDesc = { kBarrelInWhiterunBaseNew, "Skyrim.esm" };
  changeForm.inv.AddItem(0x00012eb7, 1);

  worldState.LoadChangeForm(changeForm, FormCallbacks::DoNothing());

  // Expected to ignore baseId change
  auto& refr = worldState.GetFormAt<MpObjectReference>(kBarrelInWhiterun);
  REQUIRE(refr.GetBaseId() == kBarrelInWhiterunBase);

  // But inventory and other fields still load
  REQUIRE(refr.GetInventory().GetItemCount(0x00012eb7) == 1);
}

TEST_CASE("Loads VirtualMachine with all scripts", "[WorldState]")
{
  auto& p = GetPartOne();
  p.worldState.GetPapyrusVm();
}

TEST_CASE("HasEspmFile is working correctly", "[WorldState]")
{
  WorldState worldState;
  worldState.espmFiles = { "file1", "file2" };
  REQUIRE(worldState.HasEspmFile("file1"));
  REQUIRE(worldState.HasEspmFile("file2"));
  REQUIRE_FALSE(worldState.HasEspmFile("BlowSkyrimModIndustry.exe"));
}
