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
namespace gamemode {
enum class Type
{
  Other = -1,
  Null,
  Lobby
};
}
}

namespace sweetpie {
using EventFunction = std::function<EventData(const EventData&)>;
using EventSet = std::set<EventFunction>;
using EventVector = std::vector<EventSet>;
using Listener = std::pair<EventVector, gamemode::Type>;
}

namespace sweetpie {
namespace gamemode {
class GameRules
{
public:
  EventData& operator()(const EventData& data);

private:
  std::shared_ptr<Listener> listener;
};
}
}
