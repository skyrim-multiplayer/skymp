#pragma once
#include "Data.h"
#include "Player.h"
#include "Team.h"

namespace sweetpie {
class PlaySpace
{
public:
  void AddPlayer(ID playerID);
  void RemovePlayer(ID playerID);

  float OnHit(ID aggressor, ID target, float damage);
  void OnKill(ID aggressor, ID target);

  void Update();

protected:
  void AddPlayer(Player& player);
  void RemovePlayer(Player& player);
  void MovePlayer(Player& player, const NiPoint3& pos);

  void AddTeam(Team& team);
  void RemoveTeam(Team& team);

  void AddPlayerToTeam(Player& player, Team& team);
  void RemovePlayerFromTeam(Player& player, Team& team);

  struct PlaySpaceData;
  std::shared_ptr<PlaySpaceData> data;
};
}
