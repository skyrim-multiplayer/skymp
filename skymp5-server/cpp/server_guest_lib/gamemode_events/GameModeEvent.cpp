#include "GameModeEvent.h"

#include "ScopedTask.h"
#include "WorldState.h"
#include <spdlog/spdlog.h>

bool GameModeEvent::Fire(WorldState* worldState)
{
  const char* eventName = GetName();

  worldState->currentGameModeEventsStack.push_back(this);

  Viet::ScopedTask<std::vector<GameModeEvent*>> stackPopTask(
    [](std::vector<GameModeEvent*>& stack) { stack.pop_back(); },
    worldState->currentGameModeEventsStack);

  if (!worldState) {
    spdlog::error("GameModeEvent::Fire - worldState is nullptr");
    return true;
  }

  if (spdlog::should_log(spdlog::level::trace)) {
    spdlog::trace("GameModeEvent::Fire {} {}", eventName,
                  GetArgumentsJsonArray());
  }

  bool isBlocked = false;

  for (auto& listener : worldState->listeners) {
    if (listener->OnMpApiEvent(*this) == false) {
      isBlocked = true;
    };
  }

  spdlog::trace("GameModeEvent::Fire {} {} - isBlocked: {}", eventName,
                GetArgumentsJsonArray(), isBlocked);

  if (isBlocked) {
    OnFireBlocked(worldState);
    return false;
  } else {
    OnFireSuccess(worldState);
    return true;
  }
}
