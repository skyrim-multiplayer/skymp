#pragma once
#include "IPapyrusClass.h"

class PapyrusLeveledItem final : public IPapyrusClass<PapyrusLeveledItem>
{
public:
  const char* GetName() override { return "LeveledItem"; }

  VarValue GetNthForm(VarValue slef, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
