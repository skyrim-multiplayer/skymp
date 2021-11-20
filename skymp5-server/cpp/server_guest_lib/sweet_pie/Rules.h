#pragma once
#include "EventData.h"
#include <functional>
#include <memory>
#include <set>

namespace sweetpie {
using EventFunction = std::function<EventData(const EventData&)>;
using EventSet = std::set<EventFunction>;
using EventVector = std::vector<EventSet>;
using Listener = std::pair<EventVector, gamemode::Type>;

namespace gamemode {
enum class Type
{
  Castom = -1,
  Null,
  Lobby
};

struct Rules
{
  Type type = Type::Null;
  Listener listener;
};
}
}
