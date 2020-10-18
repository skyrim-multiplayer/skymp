#include "HeuristicPolicy.h"

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

  if (!Utils::stricmp(eventName, "OnActivate") && argumentsCount >= 1)
    actor = GetFormPtr<MpActor>(arguments[0]);
}