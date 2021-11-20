#include "Data.h"
#include "EventData.h"
#include "NiPoint3.h"
#include "Team.h"
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <set>

namespace sweetpie {
using EventFunction = std::function<EventData(const EventData&)>;
using EventSet = std::set<EventFunction>;
using EventVector = std::vector<EventSet>;
}

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

namespace sweetpie {
namespace gamemode {
enum EventType : uint16_t
{
  OnHit,
  OnKill,
  end
};
enum class RuleType
{
  Other = -1,
  Null,
  Lobby
};
class GameRules
{
private:
  struct Rules;
  std::shared_ptr<Rules> rules;
};
}
}
