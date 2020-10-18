#pragma once
#include "IPapyrusCompatibilityPolicy.h"

#include "MpActor.h"
#include "MpFormGameObject.h"
#include "Utils.h"
#include "VirtualMachine.h"

class HeuristicPolicy : public IPapyrusCompatibilityPolicy
{
public:
  MpActor* GetDefaultActor() const override;

  void BeforeSendPapyrusEvent(MpForm* form, const char* eventName,
                              const VarValue* arguments,
                              size_t argumentsCount);

private:
  MpActor* actor = nullptr;
};