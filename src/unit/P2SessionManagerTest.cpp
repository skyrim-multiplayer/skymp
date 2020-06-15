#include "P2SessionManager.h"
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

void OnCustomPacket(P2SessionManager& sessionManager,
                    Networking::UserId userId, const nlohmann::json& j)
{
  simdjson::dom::parser p;
  sessionManager.OnCustomPacket(userId, p.parse(j.dump()).value());
}
}

TEST_CASE("OnConnect/OnDisconnect logging", "[P2SessionManager]")
{
  P2SessionManager::ClearDiskCache();

  LogSink sink;
  P2SessionManager sessionManager;
  sessionManager.log = sink.NewLogger();

  sessionManager.OnConnect(0);
  REQUIRE_THAT(sink.str(), Contains("Connected 0"));

  sessionManager.OnDisconnect(0);
  REQUIRE_THAT(sink.str(), Contains("Disconnected 0"));
}

TEST_CASE("Handshake: Hash hasn't been seen previously", "[P2SessionManager]")
{
  P2SessionManager::ClearDiskCache();

  LogSink sink;
  P2SessionManager sessionManager(sink);
  sessionManager.OnConnect(0);

  REQUIRE(sessionManager.sessions.empty());
  REQUIRE(sessionManager.users[0]->sessionHash.empty());

  OnCustomPacket(
    sessionManager, 0,
    { { "p", "handshake" }, { "hash", "111111111111111111111111111111" } });

  REQUIRE(sessionManager.sessions.size() == 1);
  REQUIRE(sessionManager.sessions[0].hash == "111111111111111111111111111111");
  REQUIRE(sessionManager.users[0]->sessionHash ==
          "111111111111111111111111111111");

  sessionManager.sessions.clear();
  sessionManager.LoadSessions();

  REQUIRE(sessionManager.sessions.size() == 1);
  REQUIRE(sessionManager.sessions[0].hash == "111111111111111111111111111111");
  REQUIRE(sessionManager.users[0]->sessionHash ==
          "111111111111111111111111111111");

  REQUIRE_THAT(sink.str(), Contains("Initialized new session for user 0"));
}

TEST_CASE("Handshake: Hash already used", "[P2SessionManager]")
{
  P2SessionManager::ClearDiskCache();

  LogSink sink;
  P2SessionManager sessionManager(sink);
  sessionManager.OnConnect(0);
  sessionManager.OnConnect(1);

  OnCustomPacket(
    sessionManager, 0,
    { { "p", "handshake" }, { "hash", "111111111111111111111111111111" } });

  REQUIRE(sessionManager.users[0]->sessionHash ==
          "111111111111111111111111111111");
  REQUIRE(sessionManager.users[1]->sessionHash.empty());

  OnCustomPacket(
    sessionManager, 1,
    { { "p", "handshake" }, { "hash", "111111111111111111111111111111" } });

  REQUIRE(sessionManager.sessions.size() == 1);
  REQUIRE(sessionManager.sessions[0].hash == "111111111111111111111111111111");

  REQUIRE(sessionManager.users[0]->sessionHash.empty());
  REQUIRE(sessionManager.users[1]->sessionHash ==
          "111111111111111111111111111111");

  REQUIRE_THAT(sink.str(),
               Contains("Transfer session ownership from user 0 to user 1"));
  REQUIRE_THAT(sink.str(), !Contains("Restored session for"));
}

TEST_CASE("Handshake: Restore session with such hash", "[P2SessionManager]")
{
  P2SessionManager::ClearDiskCache();

  {
    P2SessionManager sessionManager;
    P2SessionManager::SessionInfo session;
    session.bag["some_data"] = 108;
    session.hash = "222222222222222222222222222222";
    session.disconnectMoment = std::chrono::steady_clock::now();
    sessionManager.sessions.push_back(session);
  }

  LogSink sink;
  P2SessionManager sessionManager(sink);
  sessionManager.OnConnect(0);
  OnCustomPacket(
    sessionManager, 0,
    { { "p", "handshake" }, { "hash", "222222222222222222222222222222" } });
  REQUIRE(sessionManager.sessions.size() == 1);
  REQUIRE(sessionManager.sessions[0].hash == "222222222222222222222222222222");
  REQUIRE(sessionManager.sessions[0].bag["some_data"] == 108);
  REQUIRE(
    !sessionManager.sessions[0].disconnectMoment.time_since_epoch().count());
  REQUIRE(sessionManager.users[0]->sessionHash ==
          "222222222222222222222222222222");
  REQUIRE_THAT(sink.str(), Contains("Restored session for user 0"));
}

TEST_CASE("Handshake: Trying to restore an expired session destroys and "
          "recreates the session",
          "[P2SessionManager]")
{
  P2SessionManager::ClearDiskCache();

  LogSink sink;
  P2SessionManager sessionManager(sink);
  sessionManager.OnConnect(0);

  P2SessionManager::SessionInfo session;
  session.hash = "222222222222222222222222222222";
  session.bag["some_data"] = 108;
  session.disconnectMoment =
    std::chrono::steady_clock::now() - sessionManager.sessionExpiration;
  sessionManager.sessions.push_back(session);

  OnCustomPacket(
    sessionManager, 0,
    { { "p", "handshake" }, { "hash", "222222222222222222222222222222" } });

  // Check that session is existing and attached to user 0
  REQUIRE(sessionManager.sessions.size() == 1);
  REQUIRE(sessionManager.users[0]->sessionHash ==
          "222222222222222222222222222222");

  // But if it was actually recreated, its bag must be empty
  // Also disconnection moment must be zero
  REQUIRE(sessionManager.sessions[0].bag.dump() ==
          nlohmann::json::object().dump());
  REQUIRE(
    !sessionManager.sessions[0].disconnectMoment.time_since_epoch().count());

  REQUIRE_THAT(sink.str(), Contains("Initialized new session for user 0"));
}

TEST_CASE("Disconnection datetime is written to session data",
          "[P2SessionManager]")
{
  P2SessionManager::ClearDiskCache();

  P2SessionManager sessionManager;
  sessionManager.OnConnect(0);
  OnCustomPacket(
    sessionManager, 0,
    { { "p", "handshake" }, { "hash", "222222222222222222222222222222" } });

  REQUIRE(sessionManager.sessions.size() == 1);
  REQUIRE(
    !sessionManager.sessions[0].disconnectMoment.time_since_epoch().count());

  sessionManager.OnDisconnect(0);

  REQUIRE(
    sessionManager.sessions[0].disconnectMoment.time_since_epoch().count());

  // We should be able to reload these changes immediately
  P2SessionManager sessionManager2;
  sessionManager2.LoadSessions();
  REQUIRE(sessionManager2.sessions.size() == 1);
  REQUIRE(
    sessionManager2.sessions[0].disconnectMoment.time_since_epoch().count());
}

TEST_CASE("Expired sessions are dropped during save", "[P2SessionManager]")
{
  P2SessionManager::ClearDiskCache();

  P2SessionManager sessionManager;
  P2SessionManager::SessionInfo session;
  session.hash = "222222222222222222222222222222";
  session.disconnectMoment =
    std::chrono::steady_clock::now() - sessionManager.sessionExpiration;
  sessionManager.sessions.push_back(session);
  sessionManager.SaveSessions();

  REQUIRE(sessionManager.sessions.size() == 0);
}