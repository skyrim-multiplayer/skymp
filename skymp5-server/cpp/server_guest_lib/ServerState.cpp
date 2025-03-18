#include "ServerState.h"

#include <algorithm>

#include <spdlog/spdlog.h>

#include "MpActor.h"

void ServerState::Connect(Networking::UserId userId, const std::string& guid)
{
  if (userInfo[userId] != nullptr) {
    spdlog::error("ServerState::Connect: overwritten userInfo for userId={}, "
                  "old guid: {}, new guid: {}",
                  userId, userInfo[userId]->guid, guid);
  }

  userInfo[userId] = std::make_unique<UserInfo>();
  userInfo[userId]->guid = guid;

  spdlog::info("ServerState::Connect: assigning guid for userId={}: guid={}",
               userId, guid);

  if (maxConnectedId < userId) {
    maxConnectedId = userId;
  }
}

void ServerState::Disconnect(Networking::UserId userId) noexcept
{
  userInfo[userId].reset();

  if (maxConnectedId == userId) {
    auto it =
      std::find_if(userInfo.rbegin(), userInfo.rend(),
                   [](const std::unique_ptr<UserInfo>& v) { return !!v; });
    if (it != userInfo.rend()) {
      maxConnectedId = &*it - &userInfo[0];
    } else {
      maxConnectedId = 0;
    }
  }

  actorsMap.Erase(userId);
}

bool ServerState::IsConnected(Networking::UserId userId) const
{
  return userId < std::size(userInfo) && userInfo[userId];
}

MpActor* ServerState::ActorByUser(Networking::UserId userId)
{
  return actorsMap.Find(userId);
}

const std::string& ServerState::UserGuid(Networking::UserId userId)
{
  static const std::string kEmptyString;

  if (userInfo.size() <= userId || !userInfo[userId]) {
    return kEmptyString;
  }

  return userInfo[userId]->guid;
}

Networking::UserId ServerState::UserByActor(MpActor* actor)
{
  return actorsMap.Find(actor);
}

void ServerState::EnsureUserExists(Networking::UserId userId)
{
  if (userInfo.size() <= userId || !userInfo[userId]) {
    throw std::runtime_error("User with id " + std::to_string(userId) +
                             " doesn't exist");
  }
}
