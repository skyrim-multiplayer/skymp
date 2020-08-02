#pragma once
#include "Config.h"
#include <Networking.h>
#include <array>
#include <boost/bimap.hpp>
#include <memory>
#include <simdjson.h>
#include <unordered_map>

class MpActor;

struct UserInfo
{
};

using ActorsMap = boost::bimaps::bimap<Networking::UserId, MpActor*>;

class ServerState
{
public:
  ServerState() { userInfo.resize(g_maxPlayers); }

  std::vector<std::unique_ptr<UserInfo>> userInfo;
  Networking::UserId maxConnectedId = 0;
  ActorsMap actorsMap;

  void Connect(Networking::UserId userId);
  void Disconnect(Networking::UserId userId);
  bool IsConnected(Networking::UserId userId) const;

  MpActor* ActorByUser(Networking::UserId userId)
  {
    auto it = actorsMap.left.find(userId);
    if (it == actorsMap.left.end())
      return nullptr;
    return it->second;
  }

  Networking::UserId UserByActor(MpActor* actor)
  {
    auto it = actorsMap.right.find(actor);
    if (it == actorsMap.right.end())
      return Networking::InvalidUserId;
    return it->second;
  }

  void EnsureUserExists(Networking::UserId userId)
  {
    if (userInfo.size() <= userId || !userInfo[userId])
      throw std::runtime_error("User with id " + std::to_string(userId) +
                               " doesn't exist");
  }
};