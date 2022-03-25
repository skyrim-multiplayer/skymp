#include "PartOne.h"
#include "TestUtils.hpp"
#include <catch2/catch.hpp>

PartOne& GetPartOne();

TEST_CASE("Dropping an item", "[DropItemTest]")
{
    auto& partOne = GetPartOne();
    //an iron dagger
    constexpr uint32_t baseId = 0x0001397E;
    DoConnect(partOne, 0);

    partOne.CreateActor(0xff000000, {0, 0, 0}, 0, 0x3c);
    partOne.SetUserActor(0, 0xff000000);

    MpActor& ac = partOne.worldState.GetFormAt<MpActor>(0xff);

    ac.RemoveAllItems();
    REQUIRE(ac.GetInventory().GetTotalItemCount() == 0);
    ac.AddItem(baseId, 1);
    REQUIRE(ac.GetInventory().GetTotalItemCount() == 1);
    partOne.Messages().clear();
    REQUIRE(partOne.Messages().size() == 0);
    DoMessage(partOne, 0, nlohmann::json{
            { "t", MsgType::DropItem },
            { "baseId", baseId },
            /* { "count", count } */
    });
    REQUIRE(partOne.Messages().size() == 1);
    REQUIRE(ac.GetInventory().GetItemCount(baseId) == 1);
}
