#include "PartTwo.h"
#include "Exceptions.h"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace Catch;

namespace {
void OnCustomPacket(PartTwo& partTwo, Networking::UserId userId,
                    const nlohmann::json& j)
{
  simdjson::dom::parser p;
  partTwo.OnCustomPacket(userId, p.parse(j.dump()).value());
}
}

TEST_CASE("Handshake: Hash hasn't been seen previously", "[PartTwo]")
{
  PartTwo::ClearDiskCache();

  PartTwo partTwo;
  partTwo.OnConnect(0);

  REQUIRE(partTwo.sessions.empty());
  REQUIRE(partTwo.users[0]->sessionHash.empty());

  OnCustomPacket(
    partTwo, 0,
    { { "p", "handshake" }, { "hash", "111111111111111111111111111111" } });

  REQUIRE(partTwo.sessions.size() == 1);
  REQUIRE(partTwo.sessions[0]->hash == "111111111111111111111111111111");
  REQUIRE(partTwo.users[0]->sessionHash == "111111111111111111111111111111");

  partTwo.sessions.clear();
  partTwo.LoadSessions();

  REQUIRE(partTwo.sessions.size() == 1);
  REQUIRE(partTwo.sessions[0]->hash == "111111111111111111111111111111");
  REQUIRE(partTwo.users[0]->sessionHash == "111111111111111111111111111111");
}

TEST_CASE("Handshake: Hash already used", "[PartTwo]")
{
  PartTwo::ClearDiskCache();

  PartTwo partTwo;
  partTwo.OnConnect(0);
  partTwo.OnConnect(1);

  OnCustomPacket(
    partTwo, 0,
    { { "p", "handshake" }, { "hash", "111111111111111111111111111111" } });

  REQUIRE_THROWS_MATCHES(
    OnCustomPacket(
      partTwo, 1,
      { { "p", "handshake" }, { "hash", "111111111111111111111111111111" } }),
    PublicError,
    Message("Hash '111111111111111111111111111111' is already used by user "
            "with id 0"));
}

TEST_CASE("Handshake: Restore session with such hash", "[PartTwo]")
{
  PartTwo::ClearDiskCache();

  {
    PartTwo partTwo;
    auto session = std::make_shared<PartTwo::SessionInfo>();
    session->bag["some_data"] = 108;
    session->hash = "222222222222222222222222222222";
    partTwo.sessions.push_back(session);
  }

  PartTwo partTwo;
  partTwo.OnConnect(0);
  OnCustomPacket(
    partTwo, 0,
    { { "p", "handshake" }, { "hash", "222222222222222222222222222222" } });
  REQUIRE(partTwo.sessions.size() == 1);
  REQUIRE(partTwo.sessions[0]->hash == "222222222222222222222222222222");
  REQUIRE(partTwo.sessions[0]->bag["some_data"] == 108);
  REQUIRE(partTwo.users[0]->sessionHash == "222222222222222222222222222222");
}