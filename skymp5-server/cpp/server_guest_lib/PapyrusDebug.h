#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusDebug : public IPapyrusClass<PapyrusDebug>
{
public:
  const char* GetName() override { return "debug"; }

  DEFINE_STATIC_SPSNIPPET(Notification);
  DEFINE_STATIC_SPSNIPPET(MessageBox);

  VarValue SendAnimationEvent(VarValue self,
                              const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy,
                WorldState* world) override
  {
    compatibilityPolicy = policy;

    AddStatic(vm, "Notification", &PapyrusDebug::Notification);
    AddStatic(vm, "MessageBox", &PapyrusDebug::MessageBox);
    AddStatic(vm, "SendAnimationEvent", &PapyrusDebug::SendAnimationEvent);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
