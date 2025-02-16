#pragma once
#include "IPapyrusClass.h"

class PapyrusPotion final : public IPapyrusClass<PapyrusPotion>
{
public:
  const char* GetName() override { return "Potion"; }

  VarValue IsFood(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;
};
