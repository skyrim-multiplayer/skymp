#include "ScriptStorage.h"
#include "PartOne.h"
#include "TestUtils.hpp"
#include <catch2/catch.hpp>


using Catch::Matchers::Contains;

extern espm::Loader l;

espm::CompressedFieldsCache g_dummyCache;

PartOne& GetPartOne();

TEST_CASE("Should be able to harvest a Nirnroot", "[Papyrus][espm]")
{
  auto& partOne = GetPartOne();
  auto& ironArrowRefr = partOne.worldState.GetFormAt<MpObjectReference>(0x001397D);

  partOne.worldState.AddForm(
    std::make_unique<MpActor>(LocationalData{ ironArrowRefr.GetPos(), NiPoint3(),
                                              ironArrowRefr.GetCellOrWorld() },
                              FormCallbacks::DoNothing()),
    0xff000000);
  auto& actor = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  enum
  {
    NirnrootIngr = 0x59b86
  };
  REQUIRE(actor.GetInventory().GetItemCount(NirnrootIngr) == 0);
  ironArrowRefr.Activate(actor);
  REQUIRE(actor.GetInventory().GetItemCount(NirnrootIngr) == 1);
  ironArrowRefr.Activate(actor);
  REQUIRE(actor.GetInventory().GetItemCount(NirnrootIngr) == 1);

  partOne.worldState.DestroyForm(0xff000000);
}

TEST_CASE("BlockActivation", "[Papyrus][ObjectReference][espm]")
{
  PartOne p;
  p.CreateActor(0xff000001, { 0, 0, 0 }, 0, 0x3c);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000001);

  //0x001397D stands for an iron arrow
  auto& refr = CreateMpObjectReference(p, 0x001397D);

  // Trying to perform activation and fails due to unattached espm
  REQUIRE_THROWS_WITH(refr.Activate(ac), Contains("No espm attached"));

  PapyrusObjectReference().BlockActivation(refr.ToVarValue(),
                                           { VarValue(true) });

  refr.events.clear();
  // Nothing happens, but event is fired
  refr.Activate(ac);
  REQUIRE(refr.events.size() == 1);
  REQUIRE(refr.events[0] == "OnActivate");

  PapyrusObjectReference().BlockActivation(refr.ToVarValue(),
                                           { VarValue(false) });
  REQUIRE_THROWS_WITH(refr.Activate(ac), Contains("No espm attached"));
}
// TEST_CASE("")
// {
//   auto& partOne = GetPartOne();

//   partOne.Messages().clear();

//   DoConnect(partOne, 0);
//   partOne.CreateActor(0xff000000, { 22572, -8634, -3597 }, 0, 0x1a26f);
//   partOne.SetUserActor(0, 0xff000000);
//   auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);
//   ac.RemoveAllItems();

//   const auto refrId = 0x0100122a;
//   const auto MountainFlower01Purple = 0x77e1e;

//   auto it = std::find_if(
//     partOne.Messages().begin(), partOne.Messages().end(), [&](auto m) {
//       return m.reliable && m.userId == 0 && m.j["type"] == "createActor" &&
//         m.j["refrId"] == refrId && m.j["props"] == nullptr;
//     });
//   REQUIRE(it != partOne.Messages().end());

//   partOne.Messages().clear();

//   auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);
//   ref.SetRelootTime(std::chrono::milliseconds(25));

//   REQUIRE(!ref.IsHarvested());

//   DoMessage(partOne, 0,
//             nlohmann::json{
//               { "t", MsgType::Activate },
//               { "data", { { "caster", 0x14 }, { "target", refrId } } } });

//   REQUIRE(partOne.Messages().size() >= 2);
//   REQUIRE(partOne.Messages()[0].j["type"] == "setInventory");
//   REQUIRE(partOne.Messages()[0].j["inventory"].dump() ==
//           nlohmann::json({ { "entries",
//                              { { { "baseId", MountainFlower01Purple },
//                                  { "count", 1 } } } } })
//             .dump());
//   REQUIRE(partOne.Messages()[1].j["idx"] == ref.GetIdx());
//   REQUIRE(partOne.Messages()[1].j["t"] == MsgType::UpdateProperty);
//   REQUIRE(partOne.Messages()[1].j["data"] == true);
//   REQUIRE(partOne.Messages()[1].j["propName"] == "isHarvested");

//   REQUIRE(ref.IsHarvested());

//   // We should not be able to harvest already harvested items
//   REQUIRE(ac.GetInventory().GetTotalItemCount() == 1);
//   ref.Activate(ac);
//   REQUIRE(ac.GetInventory().GetTotalItemCount() == 1);

//   partOne.Tick();
//   REQUIRE(ref.IsHarvested());

//   partOne.Messages().clear();
//   std::this_thread::sleep_for(std::chrono::milliseconds(50));

//   partOne.Tick();
//   REQUIRE(!ref.IsHarvested());
//   REQUIRE(partOne.Messages().size() == 1);
//   REQUIRE(partOne.Messages()[0].j["t"] == MsgType::UpdateProperty);
//   REQUIRE(partOne.Messages()[0].j["data"] == false);
//   REQUIRE(partOne.Messages()[0].j["propName"] == "isHarvested");

//   DoDisconnect(partOne, 0);
//   partOne.DestroyActor(0xff000000);
// }