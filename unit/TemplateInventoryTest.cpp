#include "MsgType.h"
#include "ServerState.h"
#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>

PartOne& GetPartOne();

TEST_CASE("MS13BanditCampfire01 in BleakFalls should have inventory/equipment "
          "derived from NPC template",
          "[TemplateInventory]")
{
  PartOne& p = GetPartOne();
  p.worldState.npcSettings["Skyrim.esm"].spawnInInterior = true;
  p.worldState.npcEnabled = true;
  uint32_t actorId = 0x39fe4;
  auto& bandit = p.worldState.GetFormAt<MpActor>(actorId);

  REQUIRE(bandit.GetEquipment().inv.ToJson().size() > 0);

  int numWeaps = 0;
  for (auto entry : bandit.GetEquipment().inv.entries) {
    auto lookupRes =
      p.worldState.GetEspm().GetBrowser().LookupById(entry.baseId);
    if (lookupRes.rec && lookupRes.rec->GetType() == "WEAP") {
      numWeaps++;
    }
  }

  REQUIRE(numWeaps == 1);
}
