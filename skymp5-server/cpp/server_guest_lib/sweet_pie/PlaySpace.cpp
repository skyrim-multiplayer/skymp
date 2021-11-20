#include "PlaySpace.h"
#include "GameRules.h"

namespace sweetpie {
struct PlaySpace::PlaySpaceData : public Data
{
  uint16_t numberOfTeams = 0;
  std::set<Team> teams;
  uint16_t maxNumberOfPlayers = 0;
  std::set<Player> players;
  gamemode::GameRules rules;
  std::set<Effect> effects;
};
}
