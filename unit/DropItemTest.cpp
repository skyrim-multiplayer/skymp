#include "MpChangeForms.h"
#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "MpObjectReference.h"
#include "PartOne.h"

PartOne& GetPartOne();

TEST_CASE("Dropping an item", "[DropItemTest]")
{
  auto& partOne = GetPartOne();
  // an iron dagger
  constexpr uint32_t ironDagger = 0x0001397E;
  constexpr uint32_t healingPotion = 0x0003EADD;
  DoConnect(partOne, 0);

  partOne.CreateActor(0xff000000, { 1, 2, 3 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);

  MpActor& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  ac.RemoveAllItems();
  REQUIRE(ac.GetInventory().GetTotalItemCount() == 0);
  ac.AddItem(ironDagger, 1);
  REQUIRE(ac.GetInventory().GetTotalItemCount() == 1);
  partOne.Messages().clear();
  REQUIRE(partOne.Messages().size() == 0);
  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::DropItem },
                            { "baseId", ironDagger },
                            { "count", 1 } });
  // 1 message from here and another 1 is comming from actionListener
  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(ac.GetInventory().GetItemCount(ironDagger) == 0);
  // TODO(#1141): reimplement spawning items
  ac.AddItem(healingPotion, 5);
  partOne.Messages().clear();
  REQUIRE(partOne.Messages().size() == 0);
  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::DropItem },
                            { "baseId", healingPotion },
                            { "count", 5 } });
  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(ac.GetInventory().GetItemCount(healingPotion) == 0);
}
