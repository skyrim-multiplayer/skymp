#include "P2SessionManager.h"
#include "Exceptions.h"
#include "JsonUtils.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <simdjson.h>

// It should not be a static variable. P2SessionManager may be static too and
// destructor calling order would be unpredictable (~P2SessionManager depends
// on g_sessionsFilePath)
#define g_sessionsFilePath                                                    \
  (std::filesystem::current_path() / "server" / "sessions")

namespace {
bool IsExpired(P2SessionManager* p,
               const P2SessionManager::SessionInfo& session)
{
  return std::chrono::steady_clock::now() - session.disconnectMoment >=
    p->sessionExpiration &&
    session.disconnectMoment.time_since_epoch().count() > 0;
};

void RemoveExpiredSessions(P2SessionManager* p)
{
  p->sessions.erase(std::remove_if(p->sessions.begin(), p->sessions.end(),
                                   [p](auto& s) { return IsExpired(p, s); }),
                    p->sessions.end());
}

std::string SerializeSessions(P2SessionManager* p)
{
  auto j = nlohmann::json::array();
  for (auto& session : p->sessions)
    j.push_back(nlohmann::json{
      { "hash", session.hash },
      { "bag", session.bag },
      { "disconnectionDateTime",
        session.disconnectMoment.time_since_epoch().count() } });
  return j.dump(2);
}

void DeserializeSessions(P2SessionManager* p, const std::string& data)
{
  p->sessions.clear();

  simdjson::dom::parser parser;
  auto jData = parser.parse(data).value().get<simdjson::dom::array>();
  for (auto& jSession : jData) {
    const char* hash;
    Read(jSession, "hash", &hash);

    int64_t disconnectionDateTime;
    Read(jSession, "disconnectionDateTime", &disconnectionDateTime);

    auto bag =
      nlohmann::json::parse((std::string)simdjson::minify(jSession["bag"]));

    p->sessions.push_back(
      { hash, bag,
        std::chrono::steady_clock::time_point(
          std::chrono::steady_clock::duration(disconnectionDateTime)) });
  }
}
}

void P2SessionManager::ClearDiskCache()
{
  std::filesystem::remove_all(std::filesystem::current_path() / "server");
}

P2SessionManager::P2SessionManager(std::shared_ptr<spdlog::logger> logger)
{
  // We expect log to always be non-nullptr
  log = logger ? logger : std::make_shared<spdlog::logger>("dummy");

  users.resize(65536);
  LoadSessions();
}

P2SessionManager::~P2SessionManager()
{
  SaveSessions();
}

void P2SessionManager::OnConnect(Networking::UserId userId)
{
  users[userId] = UserInfo();
  log->info("Connected {}", userId);
}

void P2SessionManager::OnDisconnect(Networking::UserId userId)
{
  if (auto& user = users[userId]) {
    if (!user->sessionHash.empty()) {
      auto hash = user->sessionHash.data();
      auto session = std::find_if(
        sessions.begin(), sessions.end(),
        [&](const SessionInfo& session) { return hash == session.hash; });
      if (session != sessions.end()) {
        session->disconnectMoment = std::chrono::steady_clock::now();
        SaveSessions();
      }
    }
    users[userId] = std::nullopt;
  }
  log->info("Disconnected {}", userId);
}

void P2SessionManager::OnCustomPacket(Networking::UserId userId,
                                      const simdjson::dom::element& content)
{
  const char* purpose;
  Read(content, "p", &purpose);

  if (!strcmp(purpose, "handshake")) {
    const char* hash;
    Read(content, "hash", &hash);

    auto it = std::find_if(users.begin(), users.end(),
                           [&](const std::optional<UserInfo>& user) {
                             return user && user->sessionHash == hash;
                           });
    if (it != users.end()) {
      auto newUser = userId;
      auto oldUser = it - users.begin();
      log->info("Transfer session ownership from user {} to user {}", oldUser,
                newUser);
      users[newUser]->sessionHash = std::move(users[oldUser]->sessionHash);
      users[oldUser]->sessionHash.clear();
      return;
    }

    auto existingSession = std::find_if(
      sessions.begin(), sessions.end(),
      [hash](const SessionInfo& session) { return session.hash == hash; });
    if (existingSession == sessions.end()) {
      sessions.push_back({ hash });
      log->info("Initialized new session for user {}", userId);

      SaveSessions();
    } else {
      if (IsExpired(this, *existingSession)) {
        existingSession->bag = SessionInfo().bag;
        log->info("Initialized new session for user {}", userId);
      } else
        log->info("Restored session for user {}", userId);

      existingSession->disconnectMoment -=
        existingSession->disconnectMoment.time_since_epoch();
    }

    users[userId]->sessionHash = hash;
  }
}

void P2SessionManager::LoadSessions()
{
  if (!std::filesystem::exists(g_sessionsFilePath))
    return;

  std::ifstream t(g_sessionsFilePath);
  if (!t.is_open())
    throw std::runtime_error("File " + g_sessionsFilePath.string() +
                             " is failed to open");
  std::stringstream buffer;
  buffer << t.rdbuf();
  DeserializeSessions(this, buffer.str());
}

void P2SessionManager::SaveSessions()
{
  RemoveExpiredSessions(this);
  std::filesystem::create_directories(g_sessionsFilePath.parent_path());
  std::ofstream t(g_sessionsFilePath);
  t << SerializeSessions(this);
  if (!t.good())
    throw std::runtime_error("File " + g_sessionsFilePath.string() +
                             " is not good");
}