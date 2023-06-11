#include "ServerState.h"
#include "Exceptions.h"
#include "JsonUtils.h"
#include "MpActor.h"
#include "MsgType.h"
#include <algorithm>

ServerState::ServerState()
{
  connectionMask.resize(kMaxPlayers);
  entities.resize(kMaxPlayers, null_entity);
}

void ServerState::Connect(Networking::UserId userId) noexcept
{
  // QUESTION userId > kMaxPlayers
  connectionMask[userId] = true;
  maxConnectedId = maxConnectedId < userId ? userId : maxConnectedId;
}

void ServerState::Disconnect(Networking::UserId userId) noexcept
{
  connectionMask[userId] = false;
  if (maxConnectedId == userId) {
    auto it = std::find_if(connectionMask.rbegin(), connectionMask.rend(),
                           [](const bool connected) { return connected; });
    maxConnectedId =
      it != connectionMask.rend() ? connectionMask.rend() - it : 0;
  }
  entity_t formId = GetEntityByUserId(userId);
  userIdByEntity.erase(formId);
  entities[userId] = null_entity;
}

bool ServerState::IsConnected(Networking::UserId userId) const noexcept
{
  return userId < connectionMask.size() && connectionMask[userId];
}

void ServerState::EnsureUserExists(Networking::UserId userId) const
{
  if (connectionMask.size() <= userId || !connectionMask[userId])
    throw std::runtime_error(
      fmt::format("User with id {:x} doesn't exist", userId));
}

// ATTENTION
// QUESTION
Networking::UserId ServerState::GetUserIdByEntity(
  entity_t entity) const noexcept
{
  auto it = userIdByEntity.find(entity);
  if (it == userIdByEntity.end()) {
    spdlog::error("User does not exist for the form with enityt id {:x}",
                  entity);
    return;
  }
  return it->second;
}

entity_t ServerState::GetEntityByUserId(
  Networking::UserId userId) const noexcept
{
  return userId >= entities.size() ? null_entity : entities[userId];
}

bool ServerState::Valid(Networking::UserId userId) noexcept
{
  return userId != Networking::InvalidUserId;
}

bool ServerState::Valid(entity_t entity) noexcept
{
  return entity != null_entity;
}

void ServerState::Set(Networking::UserId userId, entity_t entity)
{
  if (!Valid(userId)) {
    throw std::runtime_error(
      "Trying to insert Networking::InvalidUserId into ServerState");
  }

  if (!Valid(entity)) {
    throw std::runtime_error("Trying to insert nullptr into ServerState");
  }

  if (userId >= entities.size()) {
    throw std::runtime_error(fmt::format(
      "UserId {:x} is too big to be stored in ServerState", userId));
  }

  Erase(userId);
  Erase(entity);
  entities[userId] = entity;
  userIdByEntity[entity] = userId;
}

void ServerState::Erase(entity_t entity) noexcept
{
  Networking::UserId userId = GetUserIdByEntity(entity);
  if (Valid(userId)) {
    userIdByEntity.erase(entity);
    entities[userId] = null_entity;
  }
}

void ServerState::Erase(Networking::UserId userId) noexcept
{
  entity_t entity = GetEntityByUserId(userId);
  if (Valid(entity)) {
    userIdByEntity.erase(entity);
    entities[userId] = null_entity;
  }
}
