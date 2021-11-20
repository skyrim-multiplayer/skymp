#pragma once
#include "EventData.h"
#include "Rules.h"
#include <functional>
#include <map>
#include <memory>
#include <set>
namespace sweetpie {
namespace gamemode {
enum class Type
{
  Castom = -1,
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
struct Rules
{
  Type type = Type::Null;
  Listener listener;
};

class GameRules
{
public:
  static GameRules CreateGameRules(Type type = Type::Null);
  EventData& operator()(const EventData& data);

private:
  std::shared_ptr<Rules> rules;
};
}
}
