#include <Networking.h>
#include <catch2/catch_all.hpp>

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
