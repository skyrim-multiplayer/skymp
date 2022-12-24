#pragma once
#include "NetworkingInterface.h"
#include <unordered_map>
#include <vector>

class MpActor;

class ActorsMap
{
public:
  ActorsMap();

  // userId + actor pairs are unique
  // One user has only one actor
  void Set(Networking::UserId userId, MpActor* actor);

  void Erase(Networking::UserId userId);

  void Erase(MpActor* actor);

  // Returns Networking::InvalidUserId if no userId found
  Networking::UserId Find(MpActor* actor);

  // Returns nullptr if no actor found
  MpActor* Find(Networking::UserId userId);

private:
  std::vector<MpActor*> actorByUserId;
  std::unordered_map<MpActor*, Networking::UserId> userIdByActor;
};
