#include "NiPoint3.h"
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <set>

namespace sweetpie {
class PlaySpace
{
public:
  void AddPlayer(ID playerID);
  void RemovePlayer(ID playerID);

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

namespace sweetpie {
using ID = uint16_t;
enum InvalidId : ID
{
  InvalidID = (ID)~0
};
struct Data
{
  ID id = InvalidID;
  float score = 0;
  Space area;

  _NODISCARD constexpr bool operator==(const Data& right);
  _NODISCARD constexpr bool operator!=(const Data& right);
  _NODISCARD constexpr bool operator<(const Data& right);
  _NODISCARD constexpr bool operator>(const Data& right);
  _NODISCARD constexpr bool operator<=(const Data& right);
  _NODISCARD constexpr bool operator>=(const Data& right);
};

struct Player : public Data
{
  Team& team;
};

struct Team : public Data
{
  std::set<Player&> players;
  uint16_t maxTeamSize = 0;
};

struct Space
{
  NiPoint3 position;
  std::function<bool(const NiPoint3&)> IsInside =
    [](const NiPoint3& newPosition) { return true; };
};
}
