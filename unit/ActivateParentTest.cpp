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
namespace {
class MyListener : public PartOneListener
{
public:
  void OnConnect(Networking::UserId userId) {}
  void OnDisconnect(Networking::UserId userId) {}
  void OnCustomPacket(Networking::UserId userId,
                      const simdjson::dom::element& content)
  {
  }
  bool OnMpApiEvent(const GameModeEvent& event)
  {
    if (event.GetName() == std::string("onActivate")) {
      auto arr = nlohmann::json::parse(event.GetArgumentsJsonArray());
      uint32_t formId = arr[0].get<uint32_t>();
      numActivates[formId]++;
    }
    return true;
  }

  std::map<uint32_t, uint32_t> numActivates;
};
}

TEST_CASE("trapwallwood (54b15) in BleakFalls should be activatable by "
          "activation parents",
          "[ActivateParentTest]")
{
  PartOne& p = GetPartOne();

  auto listener = std::make_shared<MyListener>();
  p.AddListener(listener);

  p.worldState.npcSettings["Skyrim.esm"].spawnInInterior = true;
  p.worldState.npcEnabled = true;

  auto& trapWallWood = p.worldState.GetFormAt<MpObjectReference>(0x54b15);
  auto& plate = p.worldState.GetFormAt<MpObjectReference>(0x567f2);

  auto activationParents =
    espm::GetData<espm::REFR>(0x54b15, &p.worldState).activationParents;
  REQUIRE(activationParents.size() == 1);
  REQUIRE(activationParents[0].refrId == plate.GetFormId());
  REQUIRE(activationParents[0].delay == 0);

  REQUIRE(listener->numActivates[trapWallWood.GetFormId()] == 0);
  REQUIRE(listener->numActivates[plate.GetFormId()] == 0);

  plate.Activate(plate);

  REQUIRE(listener->numActivates[trapWallWood.GetFormId()] == 0);
  REQUIRE(listener->numActivates[plate.GetFormId()] == 1);

  p.Tick(); // tick timers, child activations are deferred

  REQUIRE(listener->numActivates[trapWallWood.GetFormId()] == 1);
  REQUIRE(listener->numActivates[plate.GetFormId()] == 1);
}
