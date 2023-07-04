#pragma once
#include "IPapyrusClass.h"

class PapyrusKeyword : public IPapyrusClass<PapyrusKeyword>
{
public:
  const char* GetName() override { return "keyword"; }

  VarValue GetKeyword(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    compatibilityPolicy = policy;

    AddStatic(vm, "GetKeyword", &PapyrusKeyword::GetKeyword);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
