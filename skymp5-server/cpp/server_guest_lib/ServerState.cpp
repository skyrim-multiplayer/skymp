#include "ServerState.h"

#include "Exceptions.h"
#include "JsonUtils.h"
#include "MpActor.h"
#include "MsgType.h"
#include "NetworkingInterface.h"
#include <algorithm>

ServerState::ServerState()
{
  connectionMask.resize(kMaxPlayers);
  formIds.resize(kMaxPlayers, kInvalidFormId);
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
  uint32_t formId = GetFormIdByUserId(userId);
  userIdByEntity.erase(formId);
  formIds[userId] = kInvalidFormId;
}

bool ServerState::IsConnected(Networking::UserId userId) const noexcept
{
  return userId < connectionMask.size() && connectionMask[userId];
}

void ServerState::EnsureUserExists(Networking::UserId userId) const
{
  if (connectionMask.size() <= userId || !connectionMask[userId]) {
    throw std::runtime_error(
      fmt::format("User with id {:x} doesn't exist", userId));
  }
}

// ATTENTION
// QUESTION
Networking::UserId ServerState::GetUserIdByFormId(
  uint32_t formId) const noexcept
{
  auto it = userIdByEntity.find(formId);
  if (it == userIdByEntity.end()) {
    spdlog::error("User does not exist for the form with id {:x}", formId);
    return Networking::InvalidUserId;
  }
  return it->second;
}

uint32_t ServerState::GetFormIdByUserId(
  Networking::UserId userId) const noexcept
{
  return userId >= formIds.size() ? kInvalidFormId : formIds[userId];
}

bool ServerState::Valid(Networking::UserId userId) noexcept
{
  return userId != Networking::InvalidUserId;
}

bool ServerState::Valid(uint32_t formId) noexcept
{
  return formId != kInvalidFormId;
}

void ServerState::Set(Networking::UserId userId, uint32_t formId)
{
  if (!Valid(userId)) {
    throw std::runtime_error(
      "Trying to insert Networking::InvalidUserId into ServerState");
  }

  if (!Valid(formId)) {
    throw std::runtime_error("Trying to insert nullptr into ServerState");
  }

  if (userId >= formIds.size()) {
    throw std::runtime_error(fmt::format(
      "UserId {:x} is too big to be stored in ServerState", userId));
  }

  Erase(userId);
  Erase(formId);
  formIds[userId] = formId;
  userIdByEntity[formId] = userId;
}

void ServerState::Erase(uint32_t formId) noexcept
{
  Networking::UserId userId = GetUserIdByFormId(formId);
  if (Valid(userId)) {
    userIdByEntity.erase(formId);
    formIds[userId] = kInvalidFormId;
  }
}

void ServerState::Erase(Networking::UserId userId) noexcept
{
  uint32_t formId = GetFormIdByUserId(userId);
  if (Valid(formId)) {
    userIdByEntity.erase(formId);
    formIds[userId] = kInvalidFormId;
  }
}

bool ServerState::IsUserDisconnecting(uint32_t userId) const noexcept
{
  return disconnectingUserId == userId;
}
