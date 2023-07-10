#include "NetworkingCombined.h"
#include "NetworkingMock.h"
#include <catch2/catch_all.hpp>
#include <cstring>

using namespace Networking;

#define DECLARE_CB                                                            \
  static std::vector<UserId> connected, disconnected;                         \
  static std::vector<std::pair<UserId, std::string>> messages;                \
  auto tickCb = [](void* state, UserId userId, PacketType packetType,         \
                   PacketData data, size_t length) {                          \
    if (packetType == PacketType::ServerSideUserConnect) {                    \
      connected.push_back(userId);                                            \
    } else if (packetType == PacketType::ServerSideUserDisconnect) {          \
      disconnected.push_back(userId);                                         \
    } else if (packetType == PacketType::Message) {                           \
      messages.push_back(                                                     \
        { userId, std::string((char*)data, (char*)data + length) });          \
    }                                                                         \
  };

TEST_CASE("Combined: connect/dsconnect", "[Networking]")
{
  auto s1 = std::make_shared<MockServer>();
  auto s2 = std::make_shared<MockServer>();
  auto svr = CreateCombinedServer({ s1, s2 });

  DECLARE_CB;

  auto cl1 = s1->CreateClient().first;
  auto cl2 = s2->CreateClient().first;
  svr->Tick(tickCb, nullptr);
  REQUIRE(connected == std::vector<UserId>({ 0, 1 }));
  REQUIRE(disconnected == std::vector<UserId>());

  cl1.reset();
  cl2.reset();
  svr->Tick(tickCb, nullptr);
  REQUIRE(disconnected == std::vector<UserId>({ 0, 1 }));
}

TEST_CASE("Combined: order of disconnection", "[Networking]")
{
  auto s1 = std::make_shared<MockServer>();
  auto s2 = std::make_shared<MockServer>();
  auto svr = CreateCombinedServer({ s1, s2 });

  DECLARE_CB;

  auto c0 = s2->CreateClient().first;
  auto c1 = s2->CreateClient().first;
  auto c2 = s1->CreateClient().first;
  auto c3 = s1->CreateClient().first;
  svr->Tick(tickCb, nullptr);

  REQUIRE(disconnected == std::vector<UserId>());

  c3.reset();
  c2.reset();
  c1.reset();
  c0.reset();
  svr->Tick(tickCb, nullptr);
  REQUIRE(disconnected == std::vector<UserId>({ 1, 0, 3, 2 }));
}

TEST_CASE("Combined: ids are freed", "[Networking]")
{
  auto s1 = std::make_shared<MockServer>();
  auto s2 = std::make_shared<MockServer>();
  auto svr = CreateCombinedServer({ s1, s2 });

  DECLARE_CB;

  std::shared_ptr<Networking::IClient> cl;

  for (int i = 0; i < 3; ++i) {
    auto cl = s1->CreateClient().first;
    svr->Tick(tickCb, nullptr);
    cl.reset();
    svr->Tick(tickCb, nullptr);
  }

  REQUIRE(connected == std::vector<Networking::UserId>({ 0, 0, 0 }));
  REQUIRE(disconnected == std::vector<Networking::UserId>({ 0, 0, 0 }));
}

TEST_CASE("Combined: Messages from clients are received")
{
  auto s1 = std::make_shared<MockServer>();
  auto s2 = std::make_shared<MockServer>();
  auto svr = CreateCombinedServer({ s1, s2 });

  DECLARE_CB;

  s2->CreateClient().first->Send((PacketData) "dd", 2, true);

  svr->Tick(tickCb, nullptr);

  REQUIRE(messages.size() == 1);
  REQUIRE(messages[0].first == 0);
  REQUIRE(messages[0].second == "dd");
}

TEST_CASE("Combined: Messages are transferred to clients")
{

  auto s1 = std::make_shared<MockServer>();
  auto s2 = std::make_shared<MockServer>();
  auto svr = CreateCombinedServer({ s1, s2 });

  auto cl0 = s1->CreateClient().first;
  auto cl1 = s2->CreateClient().first;

  DECLARE_CB;

  svr->Tick(tickCb, nullptr);
  svr->Send(1, (PacketData) "df", 2, true);

  static bool received = false;
  cl1->Tick(
    [](void* state, PacketType packetType, PacketData data, size_t length,
       const char* error) {
      if (data && length == 2 && !memcmp(data, "df", 2))
        received = true;
    },
    nullptr);
  REQUIRE(received);
}
