#include "PartOne.h"
#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "MpActor.h"
#include "MpObjectReference.h"

extern espm::Loader l;

PartOne& GetPartOne();

TEST_CASE("Picking up a bunch of items", "[PickUpItemCountTest]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);

  // a quiver with 15 steel arrows
  const uint32_t refrId = 0x0008a986;
  auto& refr = partOne.worldState.GetFormAt<MpObjectReference>(refrId);

  partOne.worldState.AddForm(
    std::make_unique<MpActor>(
      LocationalData{ refr.GetPos(), NiPoint3(), refr.GetCellOrWorld() },
      FormCallbacks::DoNothing()),
    0xff000000);
  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  ac.RemoveAllItems();
  REQUIRE(ac.GetInventory().GetTotalItemCount() == 0);
  refr.Activate(ac);
  REQUIRE(ac.GetInventory().GetTotalItemCount() == 15);
}
