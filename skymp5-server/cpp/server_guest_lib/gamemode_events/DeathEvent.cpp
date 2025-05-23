#include "DeathEvent.h"

#include "MpActor.h"
#include <spdlog/spdlog.h>

DeathEvent::DeathEvent(MpActor* actor_, MpActor* optionalKiller_,
                       float healthPercentageBeforeDeath_,
                       float magickaPercentageBeforeDeath_,
                       float staminaPercentageBeforeDeath_)
  : actor(actor_)
  , optionalKiller(optionalKiller_)
  , healthPercentageBeforeDeath(healthPercentageBeforeDeath_)
  , magickaPercentageBeforeDeath(magickaPercentageBeforeDeath_)
  , staminaPercentageBeforeDeath(staminaPercentageBeforeDeath_)
{
  if (!actor_) {
    spdlog::error("DeathEvent::DeathEvent - actor is nullptr");
  }
}

const char* DeathEvent::GetName() const
{
  // keep in sync with ScampServer::IsGameModeInsideDeathEventHandler
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

uint32_t DeathEvent::GetDyingActorId() const
{
  if (!actor) {
    spdlog::error("DeathEvent::GetDyingActorId - actor is nullptr");
    return 0;
  }
  return actor->GetFormId();
}

float DeathEvent::GetHealthPercentageBeforeDeath() const noexcept
{
  return healthPercentageBeforeDeath;
}

float DeathEvent::GetMagickaPercentageBeforeDeath() const noexcept
{
  return magickaPercentageBeforeDeath;
}

float DeathEvent::GetStaminaPercentageBeforeDeath() const noexcept
{
  return staminaPercentageBeforeDeath;
}

void DeathEvent::OnFireSuccess(WorldState*)
{
  if (actor) {
    actor->RespawnWithDelay();
  }
};
