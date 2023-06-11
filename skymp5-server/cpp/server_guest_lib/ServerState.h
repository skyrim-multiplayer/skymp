#pragma once
#include "Config.h"
#include <Networking.h>
#include <array>
#include <memory>
#include <simdjson.h>
#include <unordered_map>

class MpActor;

struct UserInfo
{
  bool isDisconnecting = false;
};

class ServerState final
{
public:
  ServerState();

  void Connect(Networking::UserId userId) noexcept;
  void Disconnect(Networking::UserId userId) noexcept;
  bool IsConnected(Networking::UserId userId) const noexcept;
  void EnsureUserExists(Networking::UserId userId) const;
  void Set(Networking::UserId userId, uint32_t formId);
  void Erase(Networking::UserId userId) noexcept;
  void Erase(uint32_t formId) noexcept;
  uint32_t GetFormIdByUserId(Networking::UserId userId) const noexcept;
  Networking::UserId GetUserIdByFormId(uint32_t formId) const noexcept;

public:
  static bool Valid(Networking::UserId userId) noexcept;
  static bool Valid(uint32_t formId) noexcept;

public:
  std::vector<uint32_t> formIds;
  std::unordered_map<uint32_t, Networking::UserId> userIdByEntity;
  std::vector<uint8_t> connectionMask;
  Networking::UserId maxConnectedId = 0;
  Networking::UserId disconnectingUserId = Networking::InvalidUserId;

private:
  static constexpr uint32_t kInvalidFormId = 0;
};
