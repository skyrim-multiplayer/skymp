#include "HeuristicPolicy.h"

#include "MpFormGameObject.h"

HeuristicPolicy::HeuristicPolicy(
  const std::shared_ptr<spdlog::logger>& logger_, WorldState* worldState_)
  : logger(logger_)
  , worldState(worldState_)
{
}

void HeuristicPolicy::SetDefaultActor(MpActor* newActor)
{
  actor = newActor;
}

MpActor* HeuristicPolicy::GetDefaultActor(const char* className,
                                          const char* funcName) const
{
  if (!actor && logger)
    logger->warn("Unable to determine Actor for '{}.{}' in '{}'", className,
                 funcName, currentEventName);
  return actor;
}

WorldState* HeuristicPolicy::GetWorldState() const
{
  return worldState;
}

void HeuristicPolicy::BeforeSendPapyrusEvent(MpForm* form,
                                             const char* eventName,
                                             const VarValue* arguments,
                                             size_t argumentsCount)
{
  actor = nullptr;

  if (!Utils::stricmp(eventName, "OnActivate") && argumentsCount >= 1) {
    actor = GetFormPtr<MpActor>(arguments[0]);
  } else if (!Utils::stricmp(eventName, "OnObjectEquipped") ||
             !Utils::stricmp(eventName, "OnInit") ||
             !Utils::stricmp(eventName, "OnUpdate")) {
    actor = dynamic_cast<MpActor*>(form);
  } else if ((!Utils::stricmp(eventName, "OnTriggerEnter") ||
              !Utils::stricmp(eventName, "OnTriggerLeave") ||
              !Utils::stricmp(eventName, "OnTrigger")) &&
             argumentsCount >= 1) {
    actor = GetFormPtr<MpActor>(arguments[0]);
  }

  currentEventName = eventName;
}