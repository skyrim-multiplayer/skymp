#include "Data.h"
#include "Player.h"

namespace sweetpie {
struct Team : public Data
{
  std::set<Player> players;
  uint16_t maxTeamSize = 0;
};
}
