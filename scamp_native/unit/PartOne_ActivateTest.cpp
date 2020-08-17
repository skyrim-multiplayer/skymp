#include "TestUtils.hpp"

extern espm::Loader l;
constexpr auto barrelInWhiterun = 0x4cc2d;

TEST_CASE("Activate without espm attached", "[PartOne]")
{
  PartOne partOne;
  DoConnect(partOne, 0);

  REQUIRE_THROWS_WITH(
    DoMessage(
      partOne, 0,
      nlohmann::json{ { "t", MsgType::Activate },
                      { "data", { { "caster", 0x15 }, { "target", 0 } } } }),
    Contains("No loaded esm or esp files are found"));
}

namespace {

static FakeSendTarget g_tgt;

static PartOne& GetPartOne()
{
  static std::unique_ptr<PartOne> g_partOne;
  if (!g_partOne) {
    g_partOne.reset(new PartOne);
    g_partOne->AttachEspm(&l, &g_tgt);
    g_partOne->pushedSendTarget = &g_tgt;
  }
  return *g_partOne;
}
}

TEST_CASE("Activate without Actor attached", "[PartOne]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);

  REQUIRE_THROWS_WITH(
    DoMessage(
      partOne, 0,
      nlohmann::json{ { "t", MsgType::Activate },
                      { "data", { { "caster", 0x15 }, { "target", 0 } } } }),
    Contains("Can't do this without Actor attached"));

  DoDisconnect(partOne, 0);
}

TEST_CASE("Activate with bad caster", "[PartOne]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c, &g_tgt);
  partOne.SetUserActor(0, 0xff000000, &g_tgt);

  REQUIRE_THROWS_WITH(
    DoMessage(
      partOne, 0,
      nlohmann::json{ { "t", MsgType::Activate },
                      { "data", { { "caster", 0x15 }, { "target", 0 } } } }),
    Contains("Bad caster (0x15)"));

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("Activate with incorrect WorldSpace", "[PartOne]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c, &g_tgt);
  partOne.SetUserActor(0, 0xff000000, &g_tgt);

  REQUIRE_THROWS_WITH(
    DoMessage(
      partOne, 0,
      nlohmann::json{
        { "t", MsgType::Activate },
        { "data", { { "caster", 0x14 }, { "target", barrelInWhiterun } } } }),
    Contains("WorldSpace doesn't match: caster is in Tamriel, target is in "
             "WhiterunWorld"));

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("Activation of unexisting ref doesn't throw anything", "[PartOne]")
{
  auto& partOne = GetPartOne();
  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 22572, -8634, -3597 }, 0, 0x1a26f, &g_tgt);
  partOne.SetUserActor(0, 0xff000000, &g_tgt);

  DoMessage(partOne, 0,
            nlohmann::json{
              { "t", MsgType::Activate },
              { "data", { { "caster", 0x14 }, { "target", 0xdeadbeef } } } });

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("See harvested PurpleMountainFlower in Whiterun", "[PartOne]")
{
  auto& partOne = GetPartOne();
  g_tgt = {};

  const auto refrId = 0x0100122a;
  auto& refr = partOne.worldState.GetFormAt<MpObjectReference>(refrId);

  refr.SetHarvested(true);

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 22572, -8634, -3597 }, 0, 0x1a26f, &g_tgt);
  partOne.SetUserActor(0, 0xff000000, &g_tgt);

  auto it =
    std::find_if(g_tgt.messages.begin(), g_tgt.messages.end(),
                 [&](FakeSendTarget::Message m) {
                   return m.reliable && m.userId == 0 &&
                     m.j["type"] == "createActor" && m.j["refrId"] == refrId &&
                     m.j["props"] == nlohmann::json{ { "isHarvested", true } };
                 });
  REQUIRE(it != g_tgt.messages.end());

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
  refr.SetHarvested(false);
}

TEST_CASE("Activate PurpleMountainFlower in Whiterun", "[PartOne]")
{
  auto& partOne = GetPartOne();

  g_tgt = {};

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 22572, -8634, -3597 }, 0, 0x1a26f, &g_tgt);
  partOne.SetUserActor(0, 0xff000000, &g_tgt);

  const auto refrId = 0x0100122a;
  const auto MountainFlower01Purple = 0x77e1e;

  auto it = std::find_if(g_tgt.messages.begin(), g_tgt.messages.end(),
                         [&](FakeSendTarget::Message m) {
                           return m.reliable && m.userId == 0 &&
                             m.j["type"] == "createActor" &&
                             m.j["refrId"] == refrId &&
                             m.j["props"] == nullptr;
                         });
  REQUIRE(it != g_tgt.messages.end());

  g_tgt = {};

  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);
  ref.SetRelootTime(std::chrono::milliseconds(25));

  REQUIRE(!ref.IsHarvested());

  DoMessage(partOne, 0,
            nlohmann::json{
              { "t", MsgType::Activate },
              { "data", { { "caster", 0x14 }, { "target", refrId } } } });

  REQUIRE(g_tgt.messages.size() >= 2);
  REQUIRE(g_tgt.messages[0].j["type"] == "setInventory");
  REQUIRE(g_tgt.messages[0].j["inventory"].dump() ==
          nlohmann::json({ { "entries",
                             { { { "baseId", MountainFlower01Purple },
                                 { "count", 1 } } } } })
            .dump());
  REQUIRE(g_tgt.messages[1].j["t"] == MsgType::UpdateProperty);
  REQUIRE(g_tgt.messages[1].j["data"] == true);
  REQUIRE(g_tgt.messages[1].j["propName"] == "isHarvested");

  REQUIRE(ref.IsHarvested());

  partOne.Tick();
  REQUIRE(ref.IsHarvested());

  g_tgt = {};
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  partOne.Tick();
  REQUIRE(!ref.IsHarvested());
  REQUIRE(g_tgt.messages.size() == 1);
  REQUIRE(g_tgt.messages[0].j["t"] == MsgType::UpdateProperty);
  REQUIRE(g_tgt.messages[0].j["data"] == false);
  REQUIRE(g_tgt.messages[0].j["propName"] == "isHarvested");

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}