#include "TestUtils.hpp"

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
                          { "refrId", 0xff000ABC },
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

TEST_CASE("createActor message contains look", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xff000ABC, &tgt);
  const Look look = Look::FromJson(jLook["data"]);
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

TEST_CASE("GetUserActor", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xff000000, &tgt);

  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  REQUIRE(partOne.GetUserActor(0) == 0xff000000);
  REQUIRE(partOne.serverState.ActorByUser(0) != nullptr);
  REQUIRE(partOne.serverState.UserByActor(&ac) == 0);

  DoDisconnect(partOne, 0);

  REQUIRE(partOne.serverState.ActorByUser(0) == nullptr);
  REQUIRE(partOne.serverState.UserByActor(&ac) == Networking::InvalidUserId);
  REQUIRE_THROWS_WITH(partOne.GetUserActor(0),
                      Contains("User with id 0 doesn't exist"));
}

TEST_CASE("Destroying actor in disconnect event handler", "[PartOne]")
{
  FakeSendTarget tgt;
  static PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xff000ABC, &tgt);
  DoMessage(partOne, 0, jMovement);

  static auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000ABC);

  class Listener : public PartOne::Listener
  {
  public:
    void OnConnect(Networking::UserId userId) override {}
    void OnDisconnect(Networking::UserId userId) override
    {
      REQUIRE(partOne.serverState.UserByActor(&ac) == 0);
      partOne.DestroyActor(0xff000ABC);
    }
    void OnCustomPacket(Networking::UserId userId,
                        const simdjson::dom::element& content) override
    {
    }
  };

  partOne.AddListener(std::shared_ptr<PartOne::Listener>(new Listener));

  REQUIRE(partOne.serverState.UserByActor(&ac) == 0);
  DoDisconnect(partOne, 1);
  REQUIRE(partOne.serverState.UserByActor(&ac) == Networking::InvalidUserId);
}

TEST_CASE("Bug with subscription", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  DoConnect(partOne, 0);

  partOne.CreateActor(0xff000000, { 1, 1, 1 }, 3, 0x3c, &tgt);
  partOne.SetEnabled(0xff000000, true);
  partOne.SetEnabled(0xff000000, false);
  partOne.SetEnabled(0xff000000, true);
  partOne.SetEnabled(0xff000000, false);
  partOne.SetEnabled(0xff000000, true);
  partOne.SetUserActor(0, 0xff000000, &tgt);

  REQUIRE(tgt.messages.size() == 1);
  REQUIRE(tgt.messages[0].j["type"] == "createActor");
}

TEST_CASE("SetUserActor doesn't work with disabled actors", "[PartOne]")
{
  PartOne partOne;

  REQUIRE_THROWS_WITH(partOne.GetUserActor(Networking::InvalidUserId),
                      Contains("User with id 65535 doesn't exist"));

  REQUIRE_THROWS_WITH(
    partOne.SetUserActor(Networking::InvalidUserId, 0, nullptr),
    Contains("User with id 65535 doesn't exist"));
}

TEST_CASE("Actor should see its inventory in 'createActor' message",
          "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  DoConnect(partOne, 0);

  partOne.CreateActor(0xff000000, { 1, 1, 1 }, 3, 0x3c, &tgt);
  partOne.worldState.GetFormAt<MpActor>(0xff000000).AddItem(0x12eb7, 3);
  partOne.SetUserActor(0, 0xff000000, &tgt);

  REQUIRE(tgt.messages.size() == 1);
  REQUIRE(tgt.messages[0].j["type"] == "createActor");
  REQUIRE(tgt.messages[0].j["props"]["inventory"] ==
          Inventory().AddItem(0x12eb7, 3).ToJson());
}

TEST_CASE("'isRaceMenuOpen' property should present in 'createActor'",
          "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  DoConnect(partOne, 0);

  partOne.CreateActor(0xff000000, { 1, 1, 1 }, 3, 0x3c, &tgt);
  partOne.worldState.GetFormAt<MpActor>(0xff000000).SetRaceMenuOpen(true);
  partOne.SetUserActor(0, 0xff000000, &tgt);

  REQUIRE(tgt.messages.size() == 1);
  REQUIRE(tgt.messages[0].j["type"] == "createActor");
  REQUIRE(tgt.messages[0].j["props"]["isRaceMenuOpen"] == true);
}