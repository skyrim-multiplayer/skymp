#include "IdManager.h"
#include <Networking.h>
#include <catch2/catch_all.hpp>

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
