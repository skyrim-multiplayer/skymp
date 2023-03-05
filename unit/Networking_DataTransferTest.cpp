#include <Networking.h>
#include <catch2/catch_all.hpp>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST_CASE("Data transfer", "[Networking]")
{
  static auto server = Networking::CreateServer(7778, MAX_PLAYERS);
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
