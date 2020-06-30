#include "PartOne.h"
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
  REQUIRE_THROWS_WITH(partOne.SetUserActor(549, 0xff000000, nullptr),
                      Contains("User with id 549 doesn't exist"));
  DoConnect(partOne, 549);

  REQUIRE_THROWS_WITH(partOne.SetUserActor(549, 0xff000000, nullptr),
                      Contains("Form with id ff000000 doesn't exist"));

  partOne.worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0xff000000);

  REQUIRE_THROWS_WITH(partOne.SetUserActor(549, 0xff000000, nullptr),
                      Contains("Form with id ff000000 is not Actor"));
}

TEST_CASE("Hypothesis: UpdateMovement may send nothing when actor without "
          "user present",
          "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  for (uint32_t i = 0; i < 100; ++i) {
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
  REQUIRE(tgt.messages.size() == 51); // Me and 50 other users created in loop
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