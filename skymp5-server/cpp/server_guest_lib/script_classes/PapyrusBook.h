#pragma once
#include "IPapyrusClass.h"

class PapyrusBook final : public IPapyrusClass<PapyrusBook>
{
public:
  const char* GetName() override { return "Book"; }

  VarValue GetSpell(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;
};
