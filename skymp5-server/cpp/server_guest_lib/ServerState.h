#pragma once
#include "ActorsMap.h"
#include "Aliases.h"
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
  void Set(Networking::UserId userId, entity_t entity);
  void Erase(Networking::UserId userId) noexcept;
  void Erase(entity_t entity) noexcept;
  entity_t GetEntityByUserId(Networking::UserId userId) const noexcept;
  Networking::UserId GetUserIdByEntity(entity_t entity) const noexcept;

public:
  std::vector<entity_t> entities;
  std::unordered_map<entity_t, Networking::UserId> userIdByEntity;
  std::vector<bool> connectionMask;
  Networking::UserId maxConnectedId = 0;
  Networking::UserId disconnectingUserId = Networking::InvalidUserId;

public:
  static bool Valid(Networking::UserId userId) noexcept;
  static bool Valid(entity_t entity) noexcept;
};
