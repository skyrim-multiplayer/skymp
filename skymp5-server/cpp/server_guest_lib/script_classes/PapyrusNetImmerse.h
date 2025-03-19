#pragma once
#include "IPapyrusClass.h"

class PapyrusNetImmerse final : public IPapyrusClass<PapyrusNetImmerse>
{
public:
  const char* GetName() override { return "NetImmerse"; }

  VarValue SetNodeTextureSet(VarValue self,
                             const std::vector<VarValue>& arguments);

  VarValue SetNodeScale(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;
};
