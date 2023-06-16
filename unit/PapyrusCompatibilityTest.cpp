#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

// PartOne_ActivateTest.cpp
extern espm::CompressedFieldsCache g_dummyCache;
PartOne& GetPartOne();

TEST_CASE("Should be able to harvest a Nirnroot", "[Papyrus][espm]")
{
  auto& partOne = GetPartOne();
  auto& nirnrootRef = partOne.worldState.GetFormAt<MpObjectReference>(0xa4de9);

  partOne.worldState.AddForm(
    std::make_unique<MpActor>(LocationalData{ nirnrootRef.GetPos(), NiPoint3(),
                                              nirnrootRef.GetCellOrWorld() },
                              FormCallbacks::DoNothing()),
    0xff000000);
  auto& actor = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  enum
  {
    NirnrootIngr = 0x59b86
  };
  REQUIRE(actor.GetInventory().GetItemCount(NirnrootIngr) == 0);
  nirnrootRef.Activate(actor);
  REQUIRE(actor.GetInventory().GetItemCount(NirnrootIngr) == 1);
  nirnrootRef.Activate(actor);
  REQUIRE(actor.GetInventory().GetItemCount(NirnrootIngr) == 1);

  partOne.worldState.DestroyForm(0xff000000);
}

TEST_CASE("Server crash in CallMethod", "[Papyrus][espm]")
{
  auto& partOne = GetPartOne();
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(0xd8995);
  partOne.worldState.AddForm(
    std::make_unique<MpActor>(
      LocationalData{ ref.GetPos(), NiPoint3(), ref.GetCellOrWorld() },
      FormCallbacks::DoNothing()),
    0xff000000);
  auto& actor = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  ref.Activate(actor);
  partOne.worldState.DestroyForm(0xff000000);

  REQUIRE(ref.HasScript("dunFortSnowhawkActivateIfUnlocked"));
}

TEST_CASE("Server crash in PropGet", "[Papyrus][espm]")
{
  auto& partOne = GetPartOne();
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(0xabb6f);
  partOne.worldState.AddForm(
    std::make_unique<MpActor>(
      LocationalData{ ref.GetPos(), NiPoint3(), ref.GetCellOrWorld() },
      FormCallbacks::DoNothing()),
    0xff000000);
  auto& actor = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  ref.Activate(actor);
  partOne.worldState.DestroyForm(0xff000000);

  REQUIRE(ref.HasScript("defaultDisableHavokOnLoad"));
}

// https://github.com/skyrim-multiplayer/issue-tracker/issues/11
TEST_CASE("Activate auto load door in BrokenOarGrotto01", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(87048);

  partOne.worldState.AddForm(
    std::make_unique<MpActor>(
      LocationalData{ ref.GetPos(), NiPoint3(), ref.GetCellOrWorld() },
      FormCallbacks::DoNothing()),
    0xff000000);
  auto& actor = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  ref.Activate(actor);

  REQUIRE(actor.GetCellOrWorld() == FormDesc::Tamriel());

  partOne.worldState.DestroyForm(0xff000000);

  REQUIRE(ref.HasScript("DefaultNoEnemiesFollowDoorScript"));
}

TEST_CASE("OnTriggerEnter crash in MovarthsLairExterior01", "[PartOne][espm]")
{
  auto& partOne = GetPartOne();
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(464472);

  partOne.worldState.AddForm(
    std::make_unique<MpActor>(
      LocationalData{ { 0, 0, 0 }, NiPoint3(), ref.GetCellOrWorld() },
      FormCallbacks::DoNothing()),
    0xff000000);
  auto& actor = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  actor.SetPos(ref.GetPos());
  partOne.worldState.Tick();

  partOne.worldState.DestroyForm(0xff000000);

  REQUIRE(ref.HasScript("ms14ghoruunentrance"));
}
