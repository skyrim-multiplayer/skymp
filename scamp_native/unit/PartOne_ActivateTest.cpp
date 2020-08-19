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
static espm::CompressedFieldsCache g_dummyCache;
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

/*TEST_CASE("See harvested PurpleMountainFlower in Whiterun", "[PartOne]")
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
}*/

TEST_CASE("See open DisplayCaseSmFlat01 in Whiterun", "[PartOne]")
{
  auto& partOne = GetPartOne();
  g_tgt = {};

  const auto refrId = 0x72080;
  auto& refr = partOne.worldState.GetFormAt<MpObjectReference>(refrId);

  refr.SetOpen(true);

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 25217.0293, -7373.9536, -3317.6880 }, 0,
                      0x1a26f, &g_tgt);
  partOne.SetUserActor(0, 0xff000000, &g_tgt);

  auto it =
    std::find_if(g_tgt.messages.begin(), g_tgt.messages.end(),
                 [&](FakeSendTarget::Message m) {
                   return m.reliable && m.userId == 0 &&
                     m.j["type"] == "createActor" && m.j["refrId"] == refrId &&
                     m.j["props"] == nlohmann::json{ { "isOpen", true } };
                 });
  REQUIRE(it != g_tgt.messages.end());

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
  refr.SetOpen(false);
}

TEST_CASE("Activate DisplayCaseSmFlat01 in Whiterun", "[PartOne]")
{

  auto& partOne = GetPartOne();

  g_tgt = {};

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 25217.0293, -7373.9536, -3317.6880 }, 0,
                      0x1a26f, &g_tgt);
  partOne.SetUserActor(0, 0xff000000, &g_tgt);

  const auto refrId = 0x72080;
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);

  auto it = std::find_if(g_tgt.messages.begin(), g_tgt.messages.end(),
                         [&](FakeSendTarget::Message m) {
                           return m.reliable && m.userId == 0 &&
                             m.j["type"] == "createActor" &&
                             m.j["refrId"] == refrId &&
                             m.j["props"] == nullptr;
                         });
  REQUIRE(it != g_tgt.messages.end());

  g_tgt = {};

  REQUIRE(!ref.IsOpen());
  DoMessage(partOne, 0,
            nlohmann::json{
              { "t", MsgType::Activate },
              { "data", { { "caster", 0x14 }, { "target", refrId } } } });
  REQUIRE(ref.IsOpen());
  DoMessage(partOne, 0,
            nlohmann::json{
              { "t", MsgType::Activate },
              { "data", { { "caster", 0x14 }, { "target", refrId } } } });
  REQUIRE(!ref.IsOpen());

  REQUIRE(g_tgt.messages.size() == 2);
  REQUIRE(g_tgt.messages[0].j["data"] == true);
  REQUIRE(g_tgt.messages[0].j["idx"] == ref.GetIdx());
  REQUIRE(g_tgt.messages[0].j["propName"] == "isOpen");
  REQUIRE(g_tgt.messages[0].j["t"] == MsgType::UpdateProperty);
  REQUIRE(g_tgt.messages[1].j["data"] == false);
  REQUIRE(g_tgt.messages[1].j["idx"] == ref.GetIdx());
  REQUIRE(g_tgt.messages[1].j["propName"] == "isOpen");
  REQUIRE(g_tgt.messages[1].j["t"] == MsgType::UpdateProperty);

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("Activate WRDoorMainGate01 in Whiterun", "[PartOne]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 19367.3379, -7433.0698, -3547.4492 }, 0,
                      0x1a26f, &g_tgt);
  partOne.SetUserActor(0, 0xff000000, &g_tgt);

  g_tgt = {};
  auto refrId = 0x1b1f3;
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);
  ref.SetRelootTime(std::chrono::milliseconds(30));
  DoMessage(partOne, 0,
            nlohmann::json{
              { "t", MsgType::Activate },
              { "data", { { "caster", 0x14 }, { "target", refrId } } } });
  REQUIRE(g_tgt.messages.size() >= 1);
  REQUIRE(g_tgt.messages[0].j["data"] == true);
  REQUIRE(g_tgt.messages[0].j["idx"] == ref.GetIdx());
  REQUIRE(g_tgt.messages[0].j["propName"] == "isOpen");
  REQUIRE(g_tgt.messages[0].j["t"] == MsgType::UpdateProperty);

  REQUIRE(g_tgt.messages.size() >= 2);
  REQUIRE(g_tgt.messages[1].j["type"] == "teleport");
  REQUIRE(g_tgt.messages[1].j["pos"].dump() ==
          nlohmann::json{ 19243.53515625, -7427.3427734375, -3595.4052734375 }
            .dump());
  REQUIRE(g_tgt.messages[1].j["rot"].dump() ==
          nlohmann::json{ 0.0, -0.0, -89.99922180175781 }.dump());
  REQUIRE(g_tgt.messages[1].j["worldOrCell"] == 0x3c);

  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);
  REQUIRE(ac.GetCellOrWorld() == 0x3c);

  g_tgt = {};
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  partOne.Tick();

  REQUIRE(g_tgt.messages.size() == 1);
  REQUIRE(g_tgt.messages[0].j["data"] == false);
  REQUIRE(g_tgt.messages[0].j["idx"] == ref.GetIdx());
  REQUIRE(g_tgt.messages[0].j["propName"] == "isOpen");
  REQUIRE(g_tgt.messages[0].j["t"] == MsgType::UpdateProperty);

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
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
  REQUIRE(g_tgt.messages[1].j["idx"] == ref.GetIdx());
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

TEST_CASE("BarrelFood01 PutItem/TakeItem", "[PartOne]")
{
  auto& partOne = GetPartOne();
  auto refrId = 0x20570;
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);
  REQUIRE(ref.GetInventory().IsEmpty());

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 21272.0000, -7816.0000, -3608.0000 }, 0,
                      0x1a26f, &g_tgt);
  partOne.SetUserActor(0, 0xff000000, &g_tgt);

  auto& actor = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  REQUIRE_THROWS_WITH(ref.PutItem(actor, { 0x12eb7, 2 }),
                      Contains("Actor 0xff000000 doesn't occupy ref 0x20570"));

  // Activation forces base container to be added
  g_tgt = {};
  REQUIRE(ref.GetInventory().GetTotalItemCount() == 0);
  ref.SetChanceNoneOverride(0); // LeveledItems will never produce zero items
  ref.Activate(actor);
  REQUIRE(ref.GetInventory().GetTotalItemCount() > 0);

  REQUIRE(g_tgt.messages.size() >= 1);
  REQUIRE(g_tgt.messages[0].j["t"] == MsgType::UpdateProperty);
  REQUIRE(g_tgt.messages[0].j["propName"] == "isOpen");
  REQUIRE(g_tgt.messages[0].j["idx"] == ref.GetIdx());
  REQUIRE(g_tgt.messages[0].j["data"] == true);
  REQUIRE(g_tgt.messages.size() >= 2);
  REQUIRE(g_tgt.messages[1].j["t"] == MsgType::UpdateProperty);
  REQUIRE(g_tgt.messages[1].j["propName"] == "inventory");
  REQUIRE(g_tgt.messages[1].j["idx"] == ref.GetIdx());
  REQUIRE(g_tgt.messages.size() == 3);
  REQUIRE(g_tgt.messages[2].j["type"] == "openContainer");
  REQUIRE(g_tgt.messages[2].j["target"] == ref.GetFormId());

  REQUIRE_THROWS_WITH(ref.PutItem(actor, { 0x12eb7, 2 }),
                      Contains("Source inventory doesn't have enough 0x12eb7 "
                               "(2 is required while 0 present)"));

  actor.AddItem(0x12eb7, 1);
  REQUIRE_THROWS_WITH(ref.PutItem(actor, { 0x12eb7, 2 }),
                      Contains("Source inventory doesn't have enough 0x12eb7 "
                               "(2 is required while 1 present)"));

  actor.AddItem(0x12eb7, 1);

  // On the first PutItem, we use DoMessage to ensure our messages are parsed
  // and processed successfully
  g_tgt = {};
  DoMessage(partOne, 0,
            { { "t", MsgType::PutItem },
              { "baseId", 0x12eb7 },
              { "count", 2 },
              { "target", refrId } });
  REQUIRE(g_tgt.messages.size() == 1);
  REQUIRE(g_tgt.messages[0].j["type"] == "setInventory");

  REQUIRE(actor.GetInventory().IsEmpty());
  REQUIRE(ref.GetInventory().GetItemCount(0x12eb7) == 2);

  // Keeps items from base container
  auto copy = ref.GetInventory();
  copy.RemoveItems({ { 0x12eb7, 2 } });
  REQUIRE(copy.GetTotalItemCount() > 0);

  g_tgt = {};
  DoMessage(partOne, 0,
            { { "t", MsgType::TakeItem },
              { "baseId", 0x12eb7 },
              { "count", 1 },
              { "target", refrId } });
  REQUIRE(g_tgt.messages.size() == 1);
  REQUIRE(g_tgt.messages[0].j["type"] == "setInventory");
  REQUIRE(ref.GetInventory().GetItemCount(0x12eb7) == 1);
  REQUIRE(actor.GetInventory().GetItemCount(0x12eb7) == 1);

  DoDisconnect(partOne, 0);
  partOne.DestroyActor(0xff000000);
}

TEST_CASE("Activate BarrelFood01 in Whiterun (open/close)", "[PartOne]")
{
  auto& partOne = GetPartOne();

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 21272.0000, -7816.0000, -3608.0000 }, 0,
                      0x1a26f, &g_tgt);
  partOne.SetUserActor(0, 0xff000000, &g_tgt);

  auto refrId = 0x20570;
  auto& ref = partOne.worldState.GetFormAt<MpObjectReference>(refrId);

  // Open/close container
  DoMessage(partOne, 0,
            nlohmann::json{
              { "t", MsgType::Activate },
              { "data", { { "caster", 0x14 }, { "target", refrId } } } });
  REQUIRE(ref.IsOpen());
  DoMessage(partOne, 0,
            nlohmann::json{
              { "t", MsgType::Activate },
              { "data", { { "caster", 0x14 }, { "target", refrId } } } });
  REQUIRE(!ref.IsOpen());

  DoMessage(partOne, 0,
            nlohmann::json{
              { "t", MsgType::Activate },
              { "data", { { "caster", 0x14 }, { "target", refrId } } } });
  REQUIRE(ref.IsOpen());

  class MyListener : public PartOne::Listener
  {
  public:
    void OnConnect(Networking::UserId userId) {}
    void OnDisconnect(Networking::UserId userId)
    {
      GetPartOne().DestroyActor(0xff000000);
    }
    void OnCustomPacket(Networking::UserId userId,
                        const simdjson::dom::element& content)
    {
    }
  };

  partOne.AddListener(std::make_shared<MyListener>());
  DoDisconnect(partOne, 0);

  // Actor destruction forces contaner close
  REQUIRE(!ref.IsOpen());
}