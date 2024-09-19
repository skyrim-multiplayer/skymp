#pragma once
#include "IPapyrusClass.h"

class PapyrusSkymp final : public IPapyrusClass<PapyrusSkymp>
{
public:
  const char* GetName() override { return "skymp"; }

  VarValue SetDefaultActor(VarValue self,
                           const std::vector<VarValue>& arguments);

  void Register(
    VirtualMachine& vm,
    std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy) override
  {
    policy = compatibilityPolicy;

    AddStatic(vm, "SetDefaultActor", &PapyrusSkymp::SetDefaultActor);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> policy;
};
