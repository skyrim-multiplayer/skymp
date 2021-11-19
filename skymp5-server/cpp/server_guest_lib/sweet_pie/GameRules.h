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
  void AddPlayer(Player player);
  void RemovePlayer(Player player);

  void AddTeam(Team team);
  void RemoveTeam(Team team);

  void AddPlayerToTeam(Player player, Team team);
  void RemovePlayerFromTeam(Player player, Team team);

private:
  sweetpie::Data data;
  uint16_t numberOfTeams = 0;
  std::set<std::pair<uint16_t, Team&>> teams;
  uint16_t maxNumberOfPlayers = 0;
  std::set<std::pair<uint16_t, Player&>> players;
};

struct Team : public Data
{
  std::set<std::pair<uint16_t, Player&>> players;
  uint16_t maxTeamSize = 0;
};

struct Player : public Data
{
  std::pair<uint16_t, Team&> team;
};

class Rules
{
public:
  virtual void AddPlayer(uint64_t playerID);
  virtual void RemovePlayer(uint64_t playerID);
  virtual void BeforeUpdate();
  void Update();
  virtual void OnUpdate();
  virtual void CanMove();
  void OnMove();

private:
  struct RulesImpl;
  std::shared_ptr<RulesImpl> rulesImpl;
};
}

namespace sweetpie {
namespace gamemode {
class TeamPlay : public Rules
{
private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
}
}

namespace sweetpie {
struct Data
{
  enum InvalidId : uint16_t
  {
    InvalidID = (uint16_t)~0
  };

  uint16_t id = InvalidID;
  float score = 0;
  Space bounds;
};
}

namespace sweetpie {
class Space
{
public:
  Space(const NiPoint3& position,
        std::function<bool(const NiPoint3&, const NiPoint3&)> f);
  const std::function<bool(const NiPoint3&, const NiPoint3&)> IsInside;

private:
  std::shared_ptr<const NiPoint3&> position;
};
}
