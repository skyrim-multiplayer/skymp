#pragma once
#include "IPapyrusClass.h"

class PapyrusUtility final : public IPapyrusClass<PapyrusUtility>
{
public:
  const char* GetName() override { return "Utility"; }

  VarValue Wait(VarValue self, const std::vector<VarValue>& arguments);
  VarValue RandomInt(VarValue slef, const std::vector<VarValue>& arguments);
  VarValue RandomFloat(VarValue slef, const std::vector<VarValue>& arguments);
  VarValue GetCurrentRealTime(VarValue self,
                              const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
