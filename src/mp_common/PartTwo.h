#pragma once
#include "PartOne.h"
#include <array>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

class PartTwo : public PartOne::Listener
{
public:
  static void ClearDiskCache();

  PartTwo();
  ~PartTwo();

  void OnConnect(Networking::UserId userId) override;
  void OnDisconnect(Networking::UserId userId) override;
  void OnCustomPacket(Networking::UserId userId,
                      const simdjson::dom::element& content) override;

  void LoadSessions();
  void SaveSessions();

  struct SessionInfo
  {
    std::string hash;
    nlohmann::json bag = nlohmann::json::object();
  };

  struct UserInfo
  {
    std::string sessionHash;
  };

  std::vector<std::shared_ptr<SessionInfo>> sessions;
  std::vector<std::optional<UserInfo>> users;
};