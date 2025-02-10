#include "GameModeEvent.h"

#include "WorldState.h"
#include <spdlog/spdlog.h>

bool GameModeEvent::Fire(WorldState* worldState)
{
  if (!worldState) {
    spdlog::error("GameModeEvent::Fire - worldState is nullptr");
    return true;
  }

  if (spdlog::should_log(spdlog::level::trace)) {
    spdlog::trace("GameModeEvent::Fire {} {}", GetName(),
                  GetArgumentsJsonArray());
  }

  bool isBlocked = false;

  for (auto& listener : worldState->listeners) {
    if (listener->OnMpApiEvent(*this) == false) {
      isBlocked = true;
    };
  }

  spdlog::trace("GameModeEvent::Fire {} {} - isBlocked: {}", GetName(),
                GetArgumentsJsonArray(), isBlocked);

  if (isBlocked) {
    OnFireBlocked(worldState);
    return false;
  } else {
    OnFireSuccess(worldState);
    return true;
  }
}
