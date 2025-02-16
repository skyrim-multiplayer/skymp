#pragma once
#include "IPapyrusClass.h"

class PapyrusCell final : public IPapyrusClass<PapyrusCell>
{
public:
  const char* GetName() override { return "Cell"; }

  VarValue IsAttached(VarValue self, const std::vector<VarValue>& arguments);

  VarValue IsInterior(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;
};
