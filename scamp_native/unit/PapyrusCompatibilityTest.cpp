#include "TestUtils.hpp"
#include <catch2/catch.hpp>

// PartOne_ActivateTest.cpp
extern espm::CompressedFieldsCache g_dummyCache;
extern FakeSendTarget g_tgt;
PartOne& GetPartOne();

TEST_CASE("Should be able to harvest a Nirnroot", "[Papyrus]")
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