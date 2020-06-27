#include "Networking.h"
#include "IdManager.h"
#include <catch2/catch.hpp>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST_CASE("Handler destroys the client", "[Networking]")
{
  auto server = Networking::CreateServer(7778, 100);
  static auto client = Networking::CreateClient("127.0.0.1", 7778, 500);

  static bool reset = false;

  for (int i = 0; i < 100; ++i) {
    std::this_thread::sleep_for(10ms);
    server->Tick([](auto, auto, auto, auto, auto) {}, nullptr);

    if (client)
      client->Tick(
        [](auto, auto, auto, auto, auto) {
          client.reset();
          reset = true;
        },
        nullptr);
  }

  REQUIRE(reset);
}

TEST_CASE("Connect/disconnect", "[Networking]")
{
  auto server = Networking::CreateServer(7778, 100);
  auto client = Networking::CreateClient("127.0.0.1", 7778, 500);

  REQUIRE(!client->IsConnected());
  for (int i = 0; i < 100; ++i) {
    std::this_thread::sleep_for(1ms);
    server->Tick([](auto, auto, auto, auto, auto) {}, nullptr);
    client->Tick([](auto, auto, auto, auto, auto) {}, nullptr);
  }
  REQUIRE(client->IsConnected());

  server.reset();

  std::this_thread::sleep_for(500ms * 1.5);
  client->Tick([](auto, auto, auto, auto, auto) {}, nullptr);
  REQUIRE(!client->IsConnected());
}

TEST_CASE("Data transfer", "[Networking]")
{
  static auto server = Networking::CreateServer(7778, 100);
  static auto client = Networking::CreateClient("127.0.0.1", 7778);

  std::string res;

  try {
    for (int i = 0; i < 5000; ++i) {
      std::this_thread::sleep_for(1ms);
      client->Tick(
        [](void* state, Networking::PacketType packetType,
           Networking::PacketData data, size_t length, const char* error) {
          if (packetType == Networking::PacketType::Message) {
            REQUIRE(length == 4);
            REQUIRE(!memcmp(
              data, new uint8_t[4]{ Networking::MinPacketId, 3, 2, 1 }, 4));
            throw std::runtime_error("ok");
          }
        },
        nullptr);
      if (client->IsConnected()) {
        client->Send(new uint8_t[4]{ Networking::MinPacketId, 1, 2, 3 }, 4,
                     true);
      }

      try {
        server->Tick(
          [](void* state, Networking::UserId userId,
             Networking::PacketType packetType, Networking::PacketData data,
             size_t length) {
            if (packetType == Networking::PacketType::Message) {
              REQUIRE(length == 4);
              REQUIRE(!memcmp(
                data, new uint8_t[4]{ Networking::MinPacketId, 1, 2, 3 }, 4));
              server->Send(0,
                           new uint8_t[4]{ Networking::MinPacketId, 3, 2, 1 },
                           4, true);
            }

            static bool thrown = false;
            if (!thrown) {
              thrown = true;
              throw std::logic_error(
                "some error. callback is always able to throw");
            }
          },
          nullptr);
      } catch (std::logic_error& e) {
        REQUIRE(e.what() ==
                std::string("some error. callback is always able to throw"));
      } catch (std::exception& e) {
        throw;
      }
    }
  } catch (std::exception& e) {
    res = e.what();
  }

  REQUIRE(res == "ok");

  server.reset();
  client.reset();
}

TEST_CASE("Ctors", "[Networking]")
{
  auto server = Networking::CreateServer(7778, 100);
  auto client = Networking::CreateClient("127.0.0.1", 7778);

  try {
    Networking::CreateServer(7778, 100);
    REQUIRE(false);
  } catch (std::exception& e) {
    REQUIRE(e.what() == std::string("Peer startup failed with code 5"));
  }

  try {
    Networking::CreateClient("cococo", 1);
    REQUIRE(false);
  } catch (std::exception& e) {
    REQUIRE(e.what() == std::string("Peer connect failed with code 2"));
  }

  try {
    server->Send(322, nullptr, 0, false);
    REQUIRE(false);
  } catch (std::exception& e) {
    REQUIRE(e.what() == std::string("User with id 322 doesn't exist"));
  }
}

TEST_CASE("HandlePacketServerside", "[Networking]")
{
  struct State
  {
    bool called = false;
    struct
    {
      Networking::PacketType packetType;
      Networking::PacketData data;
      size_t length;
    } callbackParams;
  } state;

  auto handler = [](void* state_, Networking::UserId userId,
                    Networking::PacketType packetType,
                    Networking::PacketData data, size_t length) {
    auto state = reinterpret_cast<State*>(state_);
    state->called = true;
    state->callbackParams.packetType = packetType;
    state->callbackParams.data = data;
    state->callbackParams.length = length;
  };

  Packet packet;
  IdManager idManager(100);

  // RakNet internal packets
  packet.data = new uint8_t[1]{ Networking::MinPacketId - 1 };
  packet.length = 1;
  Networking::HandlePacketServerside(handler, &state, &packet, idManager);
  REQUIRE(!state.called);

  // Message
  packet.data = new uint8_t[4]{ Networking::MinPacketId + 1, 1, 2, 3 };
  packet.length = 4;
  Networking::HandlePacketServerside(handler, &state, &packet, idManager);
  REQUIRE(state.called);
  REQUIRE(state.callbackParams.data == packet.data);
  REQUIRE(state.callbackParams.length == 4);
  REQUIRE(state.callbackParams.packetType == Networking::PacketType::Message);

  // Disconnection of unknown guid
  for (uint8_t t : { ID_DISCONNECTION_NOTIFICATION, ID_CONNECTION_LOST }) {
    packet.data = new uint8_t[1]{ t };
    packet.length = 1;
    packet.guid = RakNetGUID(100);
    try {
      Networking::HandlePacketServerside(handler, &state, &packet, idManager);
      REQUIRE(false);
    } catch (std::exception& e) {
      REQUIRE(
        e.what() ==
        std::string(
          "Unexpected disconnection for system without userId (guid=100)"));
    }
  }

  // Disconnection
  for (uint8_t t : { ID_DISCONNECTION_NOTIFICATION, ID_CONNECTION_LOST }) {
    static std::shared_ptr<IdManager> idManager;
    idManager.reset(new IdManager(100));

    packet.data = new uint8_t[1]{ t };
    packet.length = 1;
    packet.guid = RakNetGUID(222);
    idManager->allocateId(packet.guid);
    bool called = false;
    Networking::HandlePacketServerside(
      [](void* called, Networking::UserId userId,
         Networking::PacketType packetType, Networking::PacketData data,
         size_t length) {
        *reinterpret_cast<bool*>(called) = true;
        REQUIRE(idManager->find(0) == RakNetGUID(222));
        REQUIRE(userId == 0);
        REQUIRE(length == 0);
        REQUIRE(data == nullptr);
        REQUIRE(packetType ==
                Networking::PacketType::ServerSideUserDisconnect);
      },
      &called, &packet, *idManager);
    REQUIRE(called);
    REQUIRE(idManager->find(0) == RakNetGUID(-1));
  }

  // Connection
  {
    IdManager idm(1);
    packet.data = new uint8_t[1]{ ID_NEW_INCOMING_CONNECTION };
    packet.length = 1;
    packet.guid = RakNetGUID(222);
    bool called = false;
    Networking::HandlePacketServerside(
      [](void* called, Networking::UserId userId,
         Networking::PacketType packetType, Networking::PacketData data,
         size_t length) {
        *reinterpret_cast<bool*>(called) = true;
        REQUIRE(userId == 0);
        REQUIRE(length == 0);
        REQUIRE(data == nullptr);
        REQUIRE(packetType == Networking::PacketType::ServerSideUserConnect);
      },
      &called, &packet, idm);
    REQUIRE(called);
    REQUIRE(idm.find(RakNetGUID(222)) == 0);

    try {
      packet.guid = RakNetGUID(223);
      Networking::HandlePacketServerside(nullptr, nullptr, &packet, idm);
      REQUIRE(false);
    } catch (std::exception& e) {
      REQUIRE(e.what() == std::string("idManager is full"));
    }
  }
}

TEST_CASE("HandlePacketClientside", "[Networking]")
{
  struct State
  {
    int n = 0;

    struct
    {
      Networking::PacketType packetType;
      Networking::PacketData data;
      size_t length;
      const char* error;
    } callbackParams;
  } state;
  auto increment = [](void* state_, Networking::PacketType packetType,
                      Networking::PacketData data, size_t length,
                      const char* error) {
    auto state = reinterpret_cast<State*>(state_);
    state->n++;
    state->callbackParams.packetType = packetType;
    state->callbackParams.data = data;
    state->callbackParams.length = length;
    state->callbackParams.error = error;
  };

  Packet packet;

  packet.data = new uint8_t[1]{ Networking::MinPacketId - 1 };
  packet.length = 1;
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 0);

  packet.data = new uint8_t[4]{ Networking::MinPacketId + 1, 1, 2, 3 };
  packet.length = 4;
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 1);
  REQUIRE(state.callbackParams.data == packet.data);
  REQUIRE(state.callbackParams.error == std::string(""));
  REQUIRE(state.callbackParams.length == 4);
  REQUIRE(state.callbackParams.packetType == Networking::PacketType::Message);

  packet.data = new uint8_t[1]{ ID_DISCONNECTION_NOTIFICATION };
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 2);
  REQUIRE(state.callbackParams.packetType ==
          Networking::PacketType::ClientSideDisconnect);
  REQUIRE(state.callbackParams.error == std::string(""));

  packet.data = new uint8_t[1]{ ID_CONNECTION_LOST };
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 3);
  REQUIRE(state.callbackParams.packetType ==
          Networking::PacketType::ClientSideDisconnect);
  REQUIRE(state.callbackParams.error == std::string(""));

  packet.data = new uint8_t[1]{ ID_CONNECTION_ATTEMPT_FAILED };
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 4);
  REQUIRE(state.callbackParams.packetType ==
          Networking::PacketType::ClientSideConnectionFailed);
  REQUIRE(state.callbackParams.error == std::string(""));

  packet.data = new uint8_t[1]{ ID_ALREADY_CONNECTED };
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 5);
  REQUIRE(state.callbackParams.packetType ==
          Networking::PacketType::ClientSideConnectionDenied);
  REQUIRE(state.callbackParams.error == std::string("Already connected"));

  packet.data = new uint8_t[1]{ ID_CONNECTION_BANNED };
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 6);
  REQUIRE(state.callbackParams.packetType ==
          Networking::PacketType::ClientSideConnectionDenied);
  REQUIRE(state.callbackParams.error == std::string("Banned"));

  packet.data = new uint8_t[1]{ ID_INVALID_PASSWORD };
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 7);
  REQUIRE(state.callbackParams.packetType ==
          Networking::PacketType::ClientSideConnectionDenied);
  REQUIRE(state.callbackParams.error == std::string("Invalid password"));

  packet.data = new uint8_t[1]{ ID_INCOMPATIBLE_PROTOCOL_VERSION };
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 8);
  REQUIRE(state.callbackParams.packetType ==
          Networking::PacketType::ClientSideConnectionDenied);
  REQUIRE(state.callbackParams.error ==
          std::string("Incompatible protocol version"));

  packet.data = new uint8_t[1]{ ID_IP_RECENTLY_CONNECTED };
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 9);
  REQUIRE(state.callbackParams.packetType ==
          Networking::PacketType::ClientSideConnectionDenied);
  REQUIRE(state.callbackParams.error == std::string("IP recently connected"));

  packet.data = new uint8_t[1]{ ID_NO_FREE_INCOMING_CONNECTIONS };
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 10);
  REQUIRE(state.callbackParams.packetType ==
          Networking::PacketType::ClientSideConnectionDenied);
  REQUIRE(state.callbackParams.error ==
          std::string("No free incoming connections"));

  packet.data = new uint8_t[1]{ ID_CONNECTION_REQUEST_ACCEPTED };
  Networking::HandlePacketClientside(increment, &state, &packet);
  REQUIRE(state.n == 11);
  REQUIRE(state.callbackParams.packetType ==
          Networking::PacketType::ClientSideConnectionAccepted);
  REQUIRE(state.callbackParams.error == std::string(""));
}