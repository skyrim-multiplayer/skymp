#include "PartOne.h"
#include "MpActor.h"
#include "MsgType.h"
#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

using namespace Catch;

// Utilities for testing
namespace {
void DoMessage(PartOne& partOne, Networking::UserId id,
               const nlohmann::json& j)
{
  std::string s;
  s += (char)Networking::MinPacketId;
  s += j.dump();
  PartOne* ptr = &partOne;
  PartOne::HandlePacket(ptr, id, Networking::PacketType::Message,
                        reinterpret_cast<Networking::PacketData>(s.data()),
                        s.size());
}

void DoConnect(PartOne& partOne, Networking::UserId id)
{
  PartOne* ptr = &partOne;
  PartOne::HandlePacket(ptr, id, Networking::PacketType::ServerSideUserConnect,
                        nullptr, 0);
}

void DoDisconnect(PartOne& partOne, Networking::UserId id)
{
  PartOne* ptr = &partOne;
  PartOne::HandlePacket(
    ptr, id, Networking::PacketType::ServerSideUserDisconnect, nullptr, 0);
}

static const auto jMovement =
  nlohmann::json{ { "t", MsgType::UpdateMovement },
                  { "idx", 0 },
                  { "data",
                    { { "worldOrCell", 0x3c },
                      { "pos", { 1, -1, 1 } },
                      { "rot", { 0, 0, 179 } },
                      { "runMode", "Standing" },
                      { "direction", 0 },
                      { "isInJumpState", false },
                      { "isSneaking", false },
                      { "isBlocking", false },
                      { "isWeapDrawn", false } } } };

static const auto jLook = nlohmann::json{
  { "t", MsgType::UpdateLook },
  { "idx", 0 },
  { "data",
    { { "isFemale", false },
      { "raceId", 0x00000001 },
      { "weight", 99.9f },
      { "skinColor", -1 },
      { "hairColor", -1 },
      { "headpartIds", nlohmann::json::array() },
      { "headTextureSetId", 0x00000000 },
      { "tints", nlohmann::json::array() },
      { "name", "Oberyn" },
      { "options",
        nlohmann::json::array({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0 }) },                  // size=19
      { "presets", nlohmann::json::array({ 0, 0, 0, 0 }) } } } // size=4
};

static const auto jEquipment =
  nlohmann::json{ { "t", MsgType::UpdateEquipment },
                  { "idx", 0 },
                  { "data", { { "armor", nlohmann::json::array() } } } };

inline void DoUpdateMovement(PartOne& partOne, uint32_t actorFormId,
                             Networking::UserId userId)
{
  auto jMyMovement = jMovement;
  jMyMovement["idx"] = dynamic_cast<MpActor*>(
                         partOne.worldState.LookupFormById(actorFormId).get())
                         ->GetIdx();
  DoMessage(partOne, userId, jMyMovement);
}

class FakeListener : public PartOne::Listener
{
public:
  static std::shared_ptr<FakeListener> New()
  {
    return std::shared_ptr<FakeListener>(new FakeListener);
  }

  void OnConnect(Networking::UserId userId) override
  {
    ss << "OnConnect(" << userId << ")" << std::endl;
  }

  void OnDisconnect(Networking::UserId userId) override
  {
    ss << "OnDisconnect(" << userId << ")" << std::endl;
  }

  void OnCustomPacket(Networking::UserId userId,
                      const simdjson::dom::element& content) override
  {
    ss << "OnCustomPacket(" << userId << ", " << simdjson::minify(content)
       << ")" << std::endl;
  }

  std::string str() { return ss.str(); }

  void clear() { ss = std::stringstream(); }

private:
  std::stringstream ss;
};

class FakeSendTarget : public Networking::ISendTarget
{
public:
  void Send(Networking::UserId targetUserId, Networking::PacketData data,
            size_t length, bool reliable) override
  {
    std::string s(reinterpret_cast<const char*>(data + 1), length - 1);
    Message m{ nlohmann::json::parse(s), targetUserId, reliable };
    messages.push_back(m);
  }

  struct Message
  {
    nlohmann::json j;
    Networking::UserId userId = Networking::InvalidUserId;
    bool reliable = false;
  };

  std::vector<Message> messages;
};
}

TEST_CASE("PartOne API doesn't crash when bad userId passed", "[PartOne]")
{
  PartOne partOne;

  REQUIRE_THROWS_WITH(partOne.GetUserActor(Networking::InvalidUserId),
                      Contains("User with id 65535 doesn't exist"));

  REQUIRE_THROWS_WITH(
    partOne.SetUserActor(Networking::InvalidUserId, 0, nullptr),
    Contains("User with id 65535 doesn't exist"));
}

TEST_CASE("OnConnect/OnDisconnect", "[PartOne]")
{
  auto lst = FakeListener::New();
  PartOne partOne(lst);

  DoConnect(partOne, 0);
  DoDisconnect(partOne, 0);

  REQUIRE_THAT(lst->str(), Contains("OnConnect(0)\nOnDisconnect(0)"));
}

TEST_CASE("OnCustomPacket", "[PartOne]")
{
  auto lst = FakeListener::New();
  PartOne partOne(lst);

  DoConnect(partOne, 0);
  DoMessage(partOne, 0,
            nlohmann::json{ { "t", MsgType::CustomPacket },
                            { "content", { { "x", "y" } } } });
  REQUIRE_THAT(lst->str(), Contains("OnCustomPacket(0, {\"x\":\"y\"})"));

  REQUIRE_THROWS_WITH(
    DoMessage(partOne, 0, nlohmann::json{ { "t", MsgType::CustomPacket } }),
    Contains("Unable to read key 'content'"));
}

TEST_CASE("Messages for non-existent users", "[PartOne]")
{
  PartOne partOne;

  REQUIRE_THROWS_WITH(
    DoMessage(partOne, 0,
              nlohmann::json{ { "t", MsgType::CustomPacket },
                              { "content", { { "x", "y" } } } }),
    Contains("User with id 0 doesn't exist"));

  DoConnect(partOne, 0);

  REQUIRE_NOTHROW(
    DoMessage(partOne, 0,
              nlohmann::json{ { "t", MsgType::CustomPacket },
                              { "content", { { "x", "y" } } } }));

  DoDisconnect(partOne, 0);

  REQUIRE_THROWS_WITH(
    DoMessage(partOne, 0,
              nlohmann::json{ { "t", MsgType::CustomPacket },
                              { "content", { { "x", "y" } } } }),
    Contains("User with id 0 doesn't exist"));
}

TEST_CASE("Disconnect event sent before user actually disconnects",
          "[PartOne]")
{
  static auto partOne = std::make_shared<PartOne>();

  using Base = FakeListener;
  class MyFakeListener : public Base
  {
  public:
    void OnDisconnect(Networking::UserId userId) override
    {
      REQUIRE(partOne->IsConnected(userId));
      Base::OnDisconnect(userId);
    }
  };
  auto lst = std::make_shared<MyFakeListener>();
  partOne->AddListener(lst);

  REQUIRE(!partOne->IsConnected(0));
  DoConnect(*partOne, 0);
  REQUIRE(partOne->IsConnected(0));
  DoDisconnect(*partOne, 0);
  REQUIRE(!partOne->IsConnected(0));

  REQUIRE_THAT(lst->str(), Contains("OnConnect(0)\nOnDisconnect(0)"));
  partOne.reset();
}

TEST_CASE("CreateActor/DestroyActor", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;

  // Create:

  REQUIRE(!partOne.worldState.LookupFormById(0xff000ABC));
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);

  auto form = partOne.worldState.LookupFormById(0xff000ABC).get();
  REQUIRE(form);
  REQUIRE(form->GetFormId() == 0xff000ABC);

  auto ac = dynamic_cast<MpActor*>(form);
  REQUIRE(ac);
  REQUIRE(ac->GetPos() == NiPoint3{ 1.f, 2.f, 3.f });
  REQUIRE(ac->GetAngle() == NiPoint3{ 0.f, 0.f, 180.f });
  REQUIRE(ac->GetCellOrWorld() == 0x3c);

  // Destroy:

  partOne.DestroyActor(0xff000ABC);
  REQUIRE(!partOne.worldState.LookupFormById(0xff000ABC));
}

TEST_CASE("SetUserActor", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  DoConnect(partOne, 0);

  REQUIRE(!partOne.GetUserActor(0));
  partOne.SetUserActor(0, 0xff000ABC, &tgt);
  REQUIRE(partOne.GetUserActor(0) == 0xff000ABC);
  REQUIRE(tgt.messages.size() == 1);
  REQUIRE(tgt.messages.at(0).j.dump() ==
          nlohmann::json{ { "type", "createActor" },
                          { "idx", 0 },
                          { "isMe", true },
                          { "transform",
                            nlohmann::json{ { "pos", { 1.f, 2.f, 3.f } },
                                            { "rot", { 0.f, 0.f, 180.f } },
                                            { "worldOrCell", 0x3c } } } }
            .dump());

  // Trying to destroy actor:
  partOne.DestroyActor(0xff000ABC);
  REQUIRE(!partOne.GetUserActor(0));

  // TODO: More manipulations with actor transfer
}

TEST_CASE("Use SetUserActor with 0 formId argument", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  DoConnect(partOne, 1);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);

  REQUIRE(partOne.GetUserActor(1) == 0);
  partOne.SetUserActor(1, 0xff000ABC, &tgt);
  REQUIRE(partOne.GetUserActor(1) == 0xff000ABC);
  partOne.SetUserActor(1, 0, &tgt);
  REQUIRE(partOne.GetUserActor(1) == 0);
}

TEST_CASE("SetUserActor failures", "[PartOne]")
{
  PartOne partOne;
  REQUIRE_THROWS_WITH(partOne.SetUserActor(9, 0xff000000, nullptr),
                      Contains("User with id 9 doesn't exist"));
  DoConnect(partOne, 9);

  REQUIRE_THROWS_WITH(partOne.SetUserActor(9, 0xff000000, nullptr),
                      Contains("Form with id ff000000 doesn't exist"));

  partOne.worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0xff000000);

  REQUIRE_THROWS_WITH(partOne.SetUserActor(9, 0xff000000, nullptr),
                      Contains("Form with id ff000000 is not Actor"));
}

TEST_CASE("Hypothesis: UpdateMovement may send nothing when actor without "
          "user present",
          "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  constexpr uint32_t n = 20;
  static_assert(n <= MAX_PLAYERS - 1);

  for (uint32_t i = 0; i < n; ++i) {
    partOne.CreateActor(i + 0xff000000, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
    if (i % 2 == 0)
      continue;

    DoConnect(partOne, i + 1);
    partOne.SetUserActor(i + 1, i + 0xff000000, &tgt);

    DoUpdateMovement(partOne, i + 0xff000000, i + 1);
  }

  DoConnect(partOne, 0);
  partOne.CreateActor(0xffffffff, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xffffffff, &tgt);
  tgt = {};

  DoUpdateMovement(partOne, 0xffffffff, 0);
  REQUIRE(tgt.messages.size() == 11); // Me and 10 other users created in loop
}

TEST_CASE("SetRaceMenuOpen failures", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  partOne.CreateActor(0xff000000, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);

  REQUIRE_THROWS_WITH(
    partOne.SetRaceMenuOpen(0xff000000, true, &tgt),
    Contains("Actor with id ff000000 is not attached to any of users"));

  REQUIRE_THROWS_WITH(partOne.SetRaceMenuOpen(0xffffffff, true, &tgt),
                      Contains("Form with id ffffffff doesn't exist"));

  partOne.worldState.AddForm(std::make_unique<MpForm>(), 0xffffffff);

  REQUIRE_THROWS_WITH(partOne.SetRaceMenuOpen(0xffffffff, true, &tgt),
                      Contains("Form with id ffffffff is not Actor"));
}

TEST_CASE("SetRaceMenuOpen", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  DoConnect(partOne, 1);
  partOne.CreateActor(0xff000000, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  auto actor = std::dynamic_pointer_cast<MpActor>(
    partOne.worldState.LookupFormById(0xff000000));
  partOne.SetUserActor(1, 0xff000000, &tgt);
  tgt = {};

  REQUIRE(actor->IsRaceMenuOpen() == false);

  partOne.SetRaceMenuOpen(0xff000000, true, &tgt);

  REQUIRE(actor->IsRaceMenuOpen() == true);
  REQUIRE(tgt.messages.size() == 1);
  REQUIRE(tgt.messages[0].j ==
          nlohmann::json{ { "type", "setRaceMenuOpen" }, { "open", true } });
  REQUIRE(tgt.messages[0].userId == 1);
  REQUIRE(tgt.messages[0].reliable);

  for (int i = 0; i < 3; ++i)
    partOne.SetRaceMenuOpen(0xff000000, true, &tgt);
  REQUIRE(tgt.messages.size() == 1);

  partOne.SetRaceMenuOpen(0xff000000, false, &tgt);
  REQUIRE(tgt.messages.size() == 2);
  REQUIRE(tgt.messages[1].j ==
          nlohmann::json{ { "type", "setRaceMenuOpen" }, { "open", false } });
  REQUIRE(tgt.messages[1].userId == 1);
  REQUIRE(tgt.messages[1].reliable);

  for (int i = 0; i < 3; ++i)
    partOne.SetRaceMenuOpen(0xff000000, false, &tgt);
  REQUIRE(tgt.messages.size() == 2);
}

TEST_CASE("Look <=> JSON casts", "[PartOne]")
{
  simdjson::dom::parser p;
  auto jLookSimd = p.parse(jLook["data"].dump());
  auto look = MpActor::Look::FromJson(jLookSimd.value());

  REQUIRE(nlohmann::json::parse(look.ToJson()) == jLook["data"]);
}

TEST_CASE("UpdateLook1", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xff000ABC, &tgt);
  partOne.SetRaceMenuOpen(0xff000ABC, true, &tgt);

  DoConnect(partOne, 1);
  partOne.CreateActor(0xffABCABC, { 11.f, 22.f, 33.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(1, 0xffABCABC, &tgt);

  tgt = {};
  auto doLook = [&] { DoMessage(partOne, 0, jLook); };
  doLook();

  REQUIRE(tgt.messages.size() == 2);
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["t"] == MsgType::UpdateLook &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1 &&
                           m.j["data"] == jLook["data"];
                       }) != tgt.messages.end());

  REQUIRE(partOne.worldState.GetFormAt<MpActor>(0xff000ABC).GetLook() !=
          nullptr);
  REQUIRE(
    nlohmann::json::parse(
      partOne.worldState.GetFormAt<MpActor>(0xff000ABC).GetLookAsJson()) ==
    jLook["data"]);
}

TEST_CASE("UpdateLook2", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xff000ABC, &tgt);
  partOne.SetRaceMenuOpen(0xff000ABC, true, &tgt);

  DoConnect(partOne, 1);
  partOne.CreateActor(0xffABCABC, { 11.f, 22.f, 33.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(1, 0xffABCABC, &tgt);

  tgt = {};
  auto doLook = [&] { DoMessage(partOne, 0, jLook); };
  doLook();

  REQUIRE(tgt.messages.size() == 2);
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["t"] == MsgType::UpdateLook &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1 &&
                           m.j["data"] == jLook["data"];
                       }) != tgt.messages.end());

  REQUIRE(partOne.worldState.GetFormAt<MpActor>(0xff000ABC).GetLook() !=
          nullptr);
  REQUIRE(
    nlohmann::json::parse(
      partOne.worldState.GetFormAt<MpActor>(0xff000ABC).GetLookAsJson()) ==
    jLook["data"]);
}

TEST_CASE("UpdateEquipment", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xff000ABC, &tgt);
  DoMessage(partOne, 0, jEquipment);
  tgt = {};

  DoConnect(partOne, 1);
  partOne.CreateActor(0xffABCABC, { 11.f, 22.f, 33.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(1, 0xffABCABC, &tgt);

  // createActor should contain equipment
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "createActor" &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1 &&
                           m.j["equipment"] == jEquipment["data"];
                       }) != tgt.messages.end());
  tgt = {};

  DoMessage(partOne, 0, jEquipment);

  REQUIRE(tgt.messages.size() == 2);
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["t"] == MsgType::UpdateEquipment &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1 &&
                           m.j["data"] == jEquipment["data"];
                       }) != tgt.messages.end());
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["t"] == MsgType::UpdateEquipment &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 0 &&
                           m.j["data"] == jEquipment["data"];
                       }) != tgt.messages.end());
}

TEST_CASE("createActor message contains look", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xff000ABC, &tgt);
  const MpActor::Look look = MpActor::Look::FromJson(jLook["data"]);
  partOne.worldState.GetFormAt<MpActor>(0xff000ABC).SetLook(&look);

  DoConnect(partOne, 1);
  partOne.CreateActor(0xff000FFF, { 100.f, 200.f, 300.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(1, 0xff000FFF, &tgt);

  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "createActor" &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1 &&
                           m.j["look"] == jLook["data"];
                       }) != tgt.messages.end());

  /*REQUIRE_THROWS_WITH(
    doLook(), Contains("Unable to update appearance, RaceMenu is not open"));

  partOne.SetRaceMenuOpen(0xff000ABC, true, &tgt);
  doLook();*/
}

TEST_CASE("UpdateMovement", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  auto doMovement = [&] { DoMessage(partOne, 0, jMovement); };

  DoConnect(partOne, 0);

  doMovement();
  REQUIRE(tgt.messages.size() == 0); // No actor - no movement

  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xff000ABC, &tgt);
  tgt = {};
  doMovement();
  REQUIRE(tgt.messages.size() == 1);
  REQUIRE(tgt.messages.at(0).j.dump() == jMovement.dump());

  // UpdateMovement actually changes position and rotation
  REQUIRE(
    dynamic_cast<MpActor*>(partOne.worldState.LookupFormById(0xff000ABC).get())
      ->GetPos() == NiPoint3{ 1, -1, 1 });
  REQUIRE(
    dynamic_cast<MpActor*>(partOne.worldState.LookupFormById(0xff000ABC).get())
      ->GetAngle() == NiPoint3{ 0, 0, 179 });

  // Another player connects and see us
  DoConnect(partOne, 1);
  partOne.CreateActor(0xff00ABCD, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  tgt = {};
  partOne.SetUserActor(1, 0xff00ABCD, &tgt);
  REQUIRE(tgt.messages.size() == 3);
  // Create idx 1 for user 0, then idx 0 for 1, then idx 1 for 1 (self
  // streaming)
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "createActor" &&
                           m.j["idx"] == 1 && m.reliable && m.userId == 0;
                       }) != tgt.messages.end());
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "createActor" &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1;
                       }) != tgt.messages.end());
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "createActor" &&
                           m.j["idx"] == 1 && m.reliable && m.userId == 1;
                       }) != tgt.messages.end());

  // Look must be empty by default
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["look"] != nullptr;
                       }) == tgt.messages.end());

  tgt = {};
  doMovement();
  REQUIRE(tgt.messages.size() == 2);
  REQUIRE(tgt.messages.at(0).j.dump() == jMovement.dump());
  REQUIRE(tgt.messages.at(1).j.dump() == jMovement.dump());

  // Another player is being moved away and now doesn't see our movement
  auto acAbcd = dynamic_cast<MpActor*>(
    partOne.worldState.LookupFormById(0xff00ABCD).get());
  tgt = {};
  acAbcd->SetPos({ 100000, 0, 0 });
  REQUIRE(tgt.messages.size() == 2);
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "destroyActor" &&
                           m.j["idx"] == 1 && m.reliable && m.userId == 0;
                       }) != tgt.messages.end());
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "destroyActor" &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1;
                       }) != tgt.messages.end());
}

TEST_CASE("Server custom packet")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  DoConnect(partOne, 1);

  partOne.SendCustomPacket(1, nlohmann::json({ { "x", "y" } }).dump(), &tgt);
  REQUIRE(tgt.messages.size() == 1);
  REQUIRE(tgt.messages[0].j.dump() ==
          nlohmann::json{ { "type", "customPacket" },
                          { "content", { { "x", "y" } } } }
            .dump());
  REQUIRE(tgt.messages[0].userId == 1);
  REQUIRE(tgt.messages[0].reliable);
}