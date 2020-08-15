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

/*TEST_CASE("Activate", "[PartOne]")
{
  FakeSendTarget tgt;
  auto lst = FakeListener::New();
  PartOne partOne(lst);
  partOne.pushedSendTarget = &tgt;

  const auto barrelInWhiterun = 0x4cc2d;

  DoConnect(partOne, 0);

  REQUIRE_THROWS_WITH(
    DoMessage(
      partOne, 0,
      nlohmann::json{
        { "t", MsgType::Activate },
        { "data", { { "caster", 0x14 }, { "target", barrelInWhiterun } } } }),
    Contains("No loaded esm or esp files are found"));

  partOne.AttachEspm(&l);

  REQUIRE_THROWS_WITH(
    DoMessage(
      partOne, 0,
      nlohmann::json{
        { "t", MsgType::Activate },
        { "data", { { "caster", 0x14 }, { "target", barrelInWhiterun } } } }),
    Contains("Can't do this without Actor attached"));

  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xff000000, &tgt);

  REQUIRE_THROWS_WITH(
    DoMessage(
      partOne, 0,
      nlohmann::json{
        { "t", MsgType::Activate },
        { "data", { { "caster", 0x14 }, { "target", barrelInWhiterun } } } }),
    Contains("WorldSpace doesn't match: caster is in Tamriel, target is in "
             "WhiterunWorld"));

  // REQUIRE_THAT(lst->str(), Contains("OnCustomPacket(0, {\"x\":\"y\"})"));
}*/