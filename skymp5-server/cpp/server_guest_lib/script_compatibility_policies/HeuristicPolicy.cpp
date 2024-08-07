#include "HeuristicPolicy.h"

#include "script_objects/MpFormGameObject.h"

HeuristicPolicy::HeuristicPolicy(WorldState* worldState_)
  : worldState(worldState_)
{
}

void HeuristicPolicy::SetDefaultActor(int32_t stackId, MpActor* newActor)
{
  if (stackId < 0) {
    throw std::runtime_error("Invalid stackId was passed to SetDefaultActor");
  }

  if (stackInfo.size() <= stackId) {
    stackInfo.resize(stackId + 1);
  }
  stackInfo[stackId].ac = newActor;
}

MpActor* HeuristicPolicy::GetDefaultActor(const char* className,
                                          const char* funcName,
                                          int32_t stackId) const
{
  if (stackId < 0 || stackId >= stackInfo.size()) {
    throw std::runtime_error(
      "Invalid stackId was passed to GetDefaultActor (" +
      std::to_string(stackId) + ")");
  }

  auto& info = stackInfo[stackId];

  MpActor* actor = stackId < stackInfo.size() ? stackInfo[stackId].ac
                                              : static_cast<MpActor*>(nullptr);

  if (!actor) {
    spdlog::warn("Unable to determine Actor for '{}.{}' in '{}'", className,
                 funcName, info.currentEventName);
  }
  return actor;
}

WorldState* HeuristicPolicy::GetWorldState() const
{
  return worldState;
}

void HeuristicPolicy::BeforeSendPapyrusEvent(MpForm* form,
                                             const char* eventName,
                                             const VarValue* arguments,
                                             size_t argumentsCount,
                                             int32_t stackId)
{
  MpActor* actor = nullptr;

  if (!Utils::stricmp(eventName, "OnActivate") && argumentsCount >= 1) {
    actor = GetFormPtr<MpActor>(arguments[0]);
  } else if (!Utils::stricmp(eventName, "OnObjectEquipped") ||
             !Utils::stricmp(eventName, "OnInit") ||
             !Utils::stricmp(eventName, "OnUpdate")) {
    actor = form->AsActor();
  } else if ((!Utils::stricmp(eventName, "OnTriggerEnter") ||
              !Utils::stricmp(eventName, "OnTriggerLeave") ||
              !Utils::stricmp(eventName, "OnTrigger")) &&
             argumentsCount >= 1) {
    actor = GetFormPtr<MpActor>(arguments[0]);
  } else if (!Utils::stricmp(eventName, "OnHit") && argumentsCount >= 1) {
    actor = GetFormPtr<MpActor>(arguments[0]);
  }

  if (stackInfo.size() <= stackId) {
    stackInfo.resize(stackId + 1);
  }
  stackInfo[stackId] = { actor, eventName };
}
