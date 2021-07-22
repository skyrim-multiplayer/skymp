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
  bool isDisconnecting = false;
};

using ActorsMap = boost::bimaps::bimap<Networking::UserId, MpActor*>;

class ServerState
{
public:
  ServerState() { userInfo.resize(g_maxPlayers); }

  std::vector<std::unique_ptr<UserInfo>> userInfo;
  Networking::UserId maxConnectedId = 0;
  ActorsMap actorsMap;
  Networking::UserId disconnectingUserId = Networking::InvalidUserId;

  void Connect(Networking::UserId userId);
  void Disconnect(Networking::UserId userId) noexcept;
  bool IsConnected(Networking::UserId userId) const;
  MpActor* ActorByUser(Networking::UserId userId);
  Networking::UserId UserByActor(MpActor* actor);
  void EnsureUserExists(Networking::UserId userId);
};
