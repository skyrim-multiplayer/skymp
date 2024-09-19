#include "DeathEvent.h"

#include "MpActor.h"
#include <spdlog/spdlog.h>

DeathEvent::DeathEvent(MpActor* actor_, MpActor* optionalKiller_)
  : actor(actor_)
  , optionalKiller(optionalKiller_)
{
  if (!actor_) {
    spdlog::error("DeathEvent::DeathEvent - actor is nullptr");
  }
}

const char* DeathEvent::GetName() const
{
  return "onDeath";
}

std::string DeathEvent::GetArgumentsJsonArray() const
{
  auto actorId = actor ? actor->GetFormId() : 0;
  auto killerId = optionalKiller ? optionalKiller->GetFormId() : 0;

  std::string result;
  result += "[";
  result += std::to_string(actorId);
  result += ",";
  result += std::to_string(killerId);
  result += "]";
  return result;
}

void DeathEvent::OnFireSuccess(WorldState*)
{
  if (actor) {
    actor->RespawnWithDelay();
  }
};
