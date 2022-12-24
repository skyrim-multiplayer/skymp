#include "ServerState.h"
#include "Exceptions.h"
#include "JsonUtils.h"
#include "MpActor.h"
#include "MsgType.h"
#include <algorithm>

void ServerState::Connect(Networking::UserId userId)
{
  userInfo[userId].reset(new UserInfo);
  if (maxConnectedId < userId)
    maxConnectedId = userId;
}

void ServerState::Disconnect(Networking::UserId userId) noexcept
{
  userInfo[userId].reset();
  if (maxConnectedId == userId) {
    auto it =
      std::find_if(userInfo.rbegin(), userInfo.rend(),
                   [](const std::unique_ptr<UserInfo>& v) { return !!v; });
    if (it != userInfo.rend())
      maxConnectedId = &*it - &userInfo[0];
    else
      maxConnectedId = 0;
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

Networking::UserId ServerState::UserByActor(MpActor* actor)
{
  return actorsMap.Find(actor);
}

void ServerState::EnsureUserExists(Networking::UserId userId)
{
  if (userInfo.size() <= userId || !userInfo[userId])
    throw std::runtime_error("User with id " + std::to_string(userId) +
                             " doesn't exist");
}
