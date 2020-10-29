#include "HeuristicPolicy.h"

HeuristicPolicy::HeuristicPolicy(
  const std::shared_ptr<spdlog::logger>& logger_)
  : logger(logger_)
{
}

MpActor* HeuristicPolicy::GetDefaultActor() const
{
  return actor;
}

void HeuristicPolicy::BeforeSendPapyrusEvent(MpForm* form,
                                             const char* eventName,
                                             const VarValue* arguments,
                                             size_t argumentsCount)
{
  actor = nullptr;

  if (!Utils::stricmp(eventName, "OnActivate") && argumentsCount >= 1) {
    actor = GetFormPtr<MpActor>(arguments[0]);
  } else if (!Utils::stricmp(eventName, "OnObjectEquipped")) {
    actor = dynamic_cast<MpActor*>(form);
  } else if ((!Utils::stricmp(eventName, "OnTriggerEnter") ||
              !Utils::stricmp(eventName, "OnTriggerLeave") ||
              !Utils::stricmp(eventName, "OnTrigger")) &&
             argumentsCount >= 1) {
    actor = GetFormPtr<MpActor>(arguments[0]);
  }

  if (!actor)
    logger->warn("Unable to determine Actor for static call in '{}'",
                 eventName);
}