#include "PartOne.h"
#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "MpObjectReference.h"
#include "MpActor.h"

extern espm::Loader l;

PartOne& GetPartOne();

TEST_CASE("PickUpItemCountTest", "[PickUpItemCount]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);
  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);
  ac.RemoveAllItems();

  // a quiver with steel arrows
  constexpr uint32_t refrId = 0x000fd7;
  auto& refr = partOne.worldState.GetFormAt<MpObjectReference>(refrId);

  partOne.Messages().clear();

  REQUIRE(ac.GetInventory().GetTotalItemCount() == 1);
  refr.Activate(ac);
  REQUIRE(ac.GetInventory().GetTotalItemCount() == 6);
}
