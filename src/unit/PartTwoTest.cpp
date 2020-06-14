#include "PartTwo.h"
#include "Exceptions.h"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <spdlog/logger.h>
#include <spdlog/sinks/ostream_sink.h>

using namespace Catch;

namespace {
class LogSink
{
public:
  LogSink()
    : sink(new spdlog::sinks::ostream_sink<std::mutex>(ss, true))
  {
  }

  operator std::shared_ptr<spdlog::logger>() { return NewLogger(); }

  std::shared_ptr<spdlog::logger> NewLogger()
  {
    return std::make_shared<spdlog::logger>("test", sink);
  }

  std::string str() const { return ss.str(); }

  void clear() { ss = std::stringstream(); }

private:
  std::shared_ptr<spdlog::sinks::ostream_sink<std::mutex>> sink;
  std::stringstream ss;
};

void OnCustomPacket(PartTwo& partTwo, Networking::UserId userId,
                    const nlohmann::json& j)
{
  simdjson::dom::parser p;
  partTwo.OnCustomPacket(userId, p.parse(j.dump()).value());
}
}

TEST_CASE("OnConnect/OnDisconnect logging", "[PartTwo]")
{
  PartTwo::ClearDiskCache();

  LogSink sink;
  PartTwo partTwo;
  partTwo.log = sink.NewLogger();

  partTwo.OnConnect(0);
  REQUIRE_THAT(sink.str(), Contains("Connected 0"));

  partTwo.OnDisconnect(0);
  REQUIRE_THAT(sink.str(), Contains("Disconnected 0"));
}

TEST_CASE("Handshake: Hash hasn't been seen previously", "[PartTwo]")
{
  PartTwo::ClearDiskCache();

  LogSink sink;
  PartTwo partTwo(sink);
  partTwo.OnConnect(0);

  REQUIRE(partTwo.sessions.empty());
  REQUIRE(partTwo.users[0]->sessionHash.empty());

  OnCustomPacket(
    partTwo, 0,
    { { "p", "handshake" }, { "hash", "111111111111111111111111111111" } });

  REQUIRE(partTwo.sessions.size() == 1);
  REQUIRE(partTwo.sessions[0].hash == "111111111111111111111111111111");
  REQUIRE(partTwo.users[0]->sessionHash == "111111111111111111111111111111");

  partTwo.sessions.clear();
  partTwo.LoadSessions();

  REQUIRE(partTwo.sessions.size() == 1);
  REQUIRE(partTwo.sessions[0].hash == "111111111111111111111111111111");
  REQUIRE(partTwo.users[0]->sessionHash == "111111111111111111111111111111");

  REQUIRE_THAT(sink.str(), Contains("Initialized new session for user 0"));
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
    PartTwo::SessionInfo session;
    session.bag["some_data"] = 108;
    session.hash = "222222222222222222222222222222";
    session.disconnectMoment = std::chrono::steady_clock::now();
    partTwo.sessions.push_back(session);
  }

  LogSink sink;
  PartTwo partTwo(sink);
  partTwo.OnConnect(0);
  OnCustomPacket(
    partTwo, 0,
    { { "p", "handshake" }, { "hash", "222222222222222222222222222222" } });
  REQUIRE(partTwo.sessions.size() == 1);
  REQUIRE(partTwo.sessions[0].hash == "222222222222222222222222222222");
  REQUIRE(partTwo.sessions[0].bag["some_data"] == 108);
  REQUIRE(!partTwo.sessions[0].disconnectMoment.time_since_epoch().count());
  REQUIRE(partTwo.users[0]->sessionHash == "222222222222222222222222222222");
  REQUIRE_THAT(sink.str(), Contains("Restored session for user 0"));
}

TEST_CASE("Handshake: Trying to restore an expired session destroys and "
          "recreates the session",
          "[PartTwo]")
{
  PartTwo::ClearDiskCache();

  LogSink sink;
  PartTwo partTwo(sink);
  partTwo.OnConnect(0);

  PartTwo::SessionInfo session;
  session.hash = "222222222222222222222222222222";
  session.bag["some_data"] = 108;
  session.disconnectMoment =
    std::chrono::steady_clock::now() - partTwo.sessionExpiration;
  partTwo.sessions.push_back(session);

  OnCustomPacket(
    partTwo, 0,
    { { "p", "handshake" }, { "hash", "222222222222222222222222222222" } });

  // Check that session is existing and attached to user 0
  REQUIRE(partTwo.sessions.size() == 1);
  REQUIRE(partTwo.users[0]->sessionHash == "222222222222222222222222222222");

  // But if it was actually recreated, its bag must be empty
  // Also disconnection moment must be zero
  REQUIRE(partTwo.sessions[0].bag.dump() == nlohmann::json::object().dump());
  REQUIRE(!partTwo.sessions[0].disconnectMoment.time_since_epoch().count());

  REQUIRE_THAT(sink.str(), Contains("Initialized new session for user 0"));
}

TEST_CASE("Disconnection datetime is written to session data", "[PartTwo]")
{
  PartTwo::ClearDiskCache();

  PartTwo partTwo;
  partTwo.OnConnect(0);
  OnCustomPacket(
    partTwo, 0,
    { { "p", "handshake" }, { "hash", "222222222222222222222222222222" } });

  REQUIRE(partTwo.sessions.size() == 1);
  REQUIRE(!partTwo.sessions[0].disconnectMoment.time_since_epoch().count());

  partTwo.OnDisconnect(0);

  REQUIRE(partTwo.sessions[0].disconnectMoment.time_since_epoch().count());

  // We should be able to reload these changes immediately
  PartTwo partTwo2;
  partTwo2.LoadSessions();
  REQUIRE(partTwo2.sessions.size() == 1);
  REQUIRE(partTwo2.sessions[0].disconnectMoment.time_since_epoch().count());
}

TEST_CASE("Expired sessions are dropped during save", "[PartTwo]")
{
  PartTwo::ClearDiskCache();

  PartTwo partTwo;
  PartTwo::SessionInfo session;
  session.hash = "222222222222222222222222222222";
  session.disconnectMoment =
    std::chrono::steady_clock::now() - partTwo.sessionExpiration;
  partTwo.sessions.push_back(session);
  partTwo.SaveSessions();

  REQUIRE(partTwo.sessions.size() == 0);
}