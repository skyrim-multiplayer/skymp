#include "ServerState.h"
#include "Exceptions.h"
#include "JsonUtils.h"
#include "MsgType.h"
#include <algorithm>

void ServerState::Connect(Networking::UserId userId)
{
  userInfo[userId] = UserInfo();
  if (maxConnectedId < userId)
    maxConnectedId = userId;
}

void ServerState::Disconnect(Networking::UserId userId)
{
  userInfo[userId] = std::nullopt;
  if (maxConnectedId == userId) {
    auto it = std::find_if(
      userInfo.rbegin(), userInfo.rend(),
      [](const std::optional<UserInfo>& v) { return v.has_value(); });
    if (it != userInfo.rend())
      maxConnectedId = &*it - &userInfo[0];
    else
      maxConnectedId = 0;
  }
}

bool ServerState::IsConnected(Networking::UserId userId) const
{
  return userId < std::size(userInfo) && userInfo[userId];
}