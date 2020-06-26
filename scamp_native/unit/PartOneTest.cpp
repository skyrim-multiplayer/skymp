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
  PartOne::HandlePacket(ptr, 0, Networking::PacketType::Message,
                        reinterpret_cast<Networking::PacketData>(s.data()),
                        s.size());
}

void DoConnect(PartOne& partOne, Networking::UserId id)
{
  PartOne* ptr = &partOne;
  PartOne::HandlePacket(ptr, 0, Networking::PacketType::ServerSideUserConnect,
                        nullptr, 0);
}

void DoDisconnect(PartOne& partOne, Networking::UserId id)
{
  PartOne* ptr = &partOne;
  PartOne::HandlePacket(
    ptr, 0, Networking::PacketType::ServerSideUserDisconnect, nullptr, 0);
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