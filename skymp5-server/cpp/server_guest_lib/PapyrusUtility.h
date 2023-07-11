#pragma once
#include "IPapyrusClass.h"

class PapyrusUtility : public IPapyrusClass<PapyrusUtility>
{
public:
  const char* GetName() override { return "utility"; }

  VarValue Wait(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy,
                WorldState* world) override
  {
    compatibilityPolicy = policy;

    AddStatic(vm, "Wait", &PapyrusUtility::Wait);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
