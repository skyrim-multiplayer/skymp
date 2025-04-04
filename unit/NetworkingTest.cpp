#include "Networking.h"
#include "IdManager.h"
#include <catch2/catch_all.hpp>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST_CASE("Handler destroys the client", "[Networking]")
{
  auto server =
    Networking::CreateServer("127.0.0.1", 7778, MAX_PLAYERS, "password");
  static auto client =
    Networking::CreateClient("127.0.0.1", 7778, 500, "password");

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
  auto server =
    Networking::CreateServer("127.0.0.1", 7778, MAX_PLAYERS, "password");
  auto client = Networking::CreateClient("127.0.0.1", 7778, 500, "password");

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

TEST_CASE("Ctors", "[Networking]")
{
  auto server =
    Networking::CreateServer("127.0.0.1", 7778, MAX_PLAYERS, "password");
  auto client = Networking::CreateClient("127.0.0.1", 7778, 4000, "password");

  try {
    Networking::CreateServer("127.0.0.1", 7778, MAX_PLAYERS, "password");
    REQUIRE(false);
  } catch (std::exception& e) {
    REQUIRE(e.what() == std::string("Peer startup failed with code 5"));
  }

  try {
    Networking::CreateClient("cococo", 1, 4000, "password");
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
