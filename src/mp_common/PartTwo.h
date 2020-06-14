#pragma once
#include "PartOne.h"
#include <array>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <spdlog/logger.h>
#include <string>
#include <vector>

class PartTwo : public PartOne::Listener
{
public:
  static void ClearDiskCache();

  PartTwo(std::shared_ptr<spdlog::logger> = nullptr);
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
    std::chrono::steady_clock::time_point disconnectMoment;
  };

  struct UserInfo
  {
    std::string sessionHash;
  };

  std::vector<SessionInfo> sessions;
  std::vector<std::optional<UserInfo>> users;

  const std::chrono::steady_clock::duration sessionExpiration =
    std::chrono::seconds(5);

  std::shared_ptr<spdlog::logger> log;
};