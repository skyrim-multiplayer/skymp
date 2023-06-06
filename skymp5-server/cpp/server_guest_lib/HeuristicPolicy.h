#pragma once
#include "IPapyrusCompatibilityPolicy.h"

#include "MpActor.h"
#include "MpFormGameObject.h"
#include "papyrus-vm/Utils.h"
#include "papyrus-vm/VirtualMachine.h"
#include <spdlog/logger.h>
#include <vector>

class HeuristicPolicy : public IPapyrusCompatibilityPolicy
{
public:
  explicit HeuristicPolicy(const std::shared_ptr<spdlog::logger>& logger,
                           WorldState* worldState_);

  MpActor* GetDefaultActor(const char* className, const char* funcName,
                           int32_t stackId) const override;

  WorldState* GetWorldState() const override;

  void SetDefaultActor(int32_t stackId, MpActor* actor);

  void BeforeSendPapyrusEvent(MpForm* form, const char* eventName,
                              const VarValue* arguments, size_t argumentsCount,
                              int32_t stackId);

private:
  struct StackInfo
  {
    MpActor* ac = nullptr;
    const char* currentEventName = "";
  };
  std::vector<StackInfo> stackInfo;
  const std::shared_ptr<spdlog::logger>& logger;
  WorldState* const worldState;
};
