#pragma once
#include "IPapyrusCompatibilityPolicy.h"

#include "MpActor.h"
#include "MpFormGameObject.h"
#include "Utils.h"
#include "VirtualMachine.h"
#include <spdlog/logger.h>

class HeuristicPolicy : public IPapyrusCompatibilityPolicy
{
public:
  explicit HeuristicPolicy(const std::shared_ptr<spdlog::logger>& logger);

  MpActor* GetDefaultActor() const override;

  void BeforeSendPapyrusEvent(MpForm* form, const char* eventName,
                              const VarValue* arguments,
                              size_t argumentsCount);

private:
  MpActor* actor = nullptr;
  const std::shared_ptr<spdlog::logger>& logger;
};