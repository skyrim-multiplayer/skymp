#include "PartTwo.h"
#include "Exceptions.h"
#include "JsonUtils.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <simdjson.h>

static const std::filesystem::path g_sessionsFilePath =
  std::filesystem::current_path() / "server" / "sessions";

namespace {
std::string SerializeSessions(PartTwo* p)
{
  auto j = nlohmann::json::array();
  for (auto& session : p->sessions)
    j.push_back(
      nlohmann::json{ { "hash", session->hash }, { "bag", session->bag } });
  return j.dump(2);
}

void DeserializeSessions(PartTwo* p, const std::string& data)
{
  p->sessions.clear();

  simdjson::dom::parser parser;
  auto jData = parser.parse(data).value().get<simdjson::dom::array>();
  for (auto& jSession : jData) {
    const char* hash;
    Read(jSession, "hash", &hash);

    auto bag =
      nlohmann::json::parse((std::string)simdjson::minify(jSession["bag"]));

    auto session = std::make_shared<PartTwo::SessionInfo>();
    session->hash = hash;
    session->bag = bag;
    p->sessions.push_back(session);
  }
}
}

void PartTwo::ClearDiskCache()
{
  std::filesystem::remove_all(std::filesystem::current_path() / "server");
}

PartTwo::PartTwo()
{
  users.resize(65536);
  LoadSessions();
}

PartTwo::~PartTwo()
{
  SaveSessions();
}

void PartTwo::OnConnect(Networking::UserId userId)
{
  users[userId] = UserInfo();
}

void PartTwo::OnDisconnect(Networking::UserId userId)
{
  users[userId] = std::nullopt;
}

void PartTwo::OnCustomPacket(Networking::UserId userId,
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
      throw PublicError("Hash '" + it->value().sessionHash +
                        "' is already "
                        "used by user with id " +
                        std::to_string(it - users.begin()));
    }

    auto existingSession =
      std::find_if(sessions.begin(), sessions.end(),
                   [hash](const std::shared_ptr<SessionInfo>& session) {
                     return session->hash == hash;
                   });
    if (existingSession == sessions.end()) {
      auto session = std::make_shared<SessionInfo>();
      session->hash = hash;
      sessions.push_back(session);

      SaveSessions();
    }

    users[userId]->sessionHash = hash;
  }
}

void PartTwo::LoadSessions()
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

void PartTwo::SaveSessions()
{
  std::filesystem::create_directories(g_sessionsFilePath.parent_path());
  std::ofstream t(g_sessionsFilePath);
  t << SerializeSessions(this);
  std::cout << SerializeSessions(this);
  if (!t.good())
    throw std::runtime_error("File " + g_sessionsFilePath.string() +
                             " is not good");
}