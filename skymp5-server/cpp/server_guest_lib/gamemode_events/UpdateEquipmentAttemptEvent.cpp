#include "UpdateEquipmentAttemptEvent.h"

#include "MpActor.h"
#include <spdlog/spdlog.h>

UpdateEquipmentAttemptEvent::UpdateEquipmentAttemptEvent(
  MpActor* actor_, const Equipment& equipment_, bool isAllowed_)
  : actor(actor_)
  , equipment(equipment_)
  , isAllowed(isAllowed_)
{
}

const char* UpdateEquipmentAttemptEvent::GetName() const
{
  return "onUpdateEquipmentAttempt";
}

std::string UpdateEquipmentAttemptEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(actor->GetFormId());
  result += ",";
  result += equipment.ToJson().dump();
  result += ",";
  result += isAllowed ? "true" : "false";
  result += "]";
  return result;
}

void UpdateEquipmentAttemptEvent::OnFireSuccess(WorldState* worldState)
{
}

void UpdateEquipmentAttemptEvent::OnFireBlocked(WorldState* worldState)
{
  spdlog::warn(
    "UpdateEquipmentAttemptEvent::OnFireBlocked - Not implemented. Please "
    "consider never blocking {} event in gamemode",
    GetName());
}
