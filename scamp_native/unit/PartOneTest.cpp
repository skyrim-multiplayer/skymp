#include "TestUtils.hpp"

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