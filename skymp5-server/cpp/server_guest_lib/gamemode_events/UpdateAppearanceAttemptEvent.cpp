#include "UpdateAppearanceAttemptEvent.h"

#include "MpActor.h"
#include <spdlog/spdlog.h>

UpdateAppearanceAttemptEvent::UpdateAppearanceAttemptEvent(
  MpActor* actor_, const Appearance& appearance_, bool isAllowed_)
  : actor(actor_)
  , appearance(appearance_)
  , isAllowed(isAllowed_)
{
}

const char* UpdateAppearanceAttemptEvent::GetName() const
{
  return "onUpdateAppearanceAttempt";
}

std::string UpdateAppearanceAttemptEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(actor->GetFormId());
  result += ",";
  result += appearance.ToJson().dump();
  result += ",";
  result += isAllowed ? "true" : "false";
  result += "]";
  return result;
}

void UpdateAppearanceAttemptEvent::OnFireSuccess(WorldState* worldState)
{
}

void UpdateAppearanceAttemptEvent::OnFireBlocked(WorldState* worldState)
{
  spdlog::warn(
    "UpdateAppearanceAttemptEvent::OnFireBlocked - Not implemented. Please "
    "consider never blocking {} event in gamemode",
    GetName());
}
