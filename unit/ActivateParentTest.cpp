#include "MsgType.h"
#include "ServerState.h"
#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>

PartOne& GetPartOne();

TEST_CASE(
  "trapwallwood (54b15) in BleakFalls shouldn't be activatable by actors",
  "[ActivateParentTest]")
{
  PartOne& p = GetPartOne();

  p.worldState.npcSettings["Skyrim.esm"].spawnInInterior = true;
  p.worldState.npcEnabled = true;

  auto& trapWallWood = p.worldState.GetFormAt<MpObjectReference>(0x54b15);

  // Bandit will try to activate trapwallwood
  uint32_t actorId = 0x39fe4;
  auto& bandit = p.worldState.GetFormAt<MpActor>(actorId);
  bandit.SetPos(trapWallWood.GetPos());

  REQUIRE_THROWS_WITH(trapWallWood.Activate(bandit),
                      "Only activation parents can activate this object");
}

TEST_CASE("trapwallwood (54b15) in BleakFalls should be activatable by "
          "activation parents",
          "[ActivateParentTest]")
{
  PartOne& p = GetPartOne();

  p.worldState.npcSettings["Skyrim.esm"].spawnInInterior = true;
  p.worldState.npcEnabled = true;

  auto& trapWallWood = p.worldState.GetFormAt<MpObjectReference>(0x54b15);
  auto& plate = p.worldState.GetFormAt<MpObjectReference>(0x567f2);

  auto activationParents =
    espm::GetData<espm::REFR>(0x54b15, &p.worldState).activationParents;
  REQUIRE(activationParents.size() == 1);
  REQUIRE(activationParents[0].refrId == plate.GetFormId());
  REQUIRE(activationParents[0].delay == 0);

  // TODO: test (and implement) activation parenting
}
