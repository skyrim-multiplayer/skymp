#include "GameRules.h"

namespace sweetpie {
namespace gamemode {
EventData& GameRules::operator()(const EventData& data)
{
  EventData result = EventData(data);
  for (auto& f : listener->first[data.type]) {
    result = f(result);
  };

  return result;
}
}
}
