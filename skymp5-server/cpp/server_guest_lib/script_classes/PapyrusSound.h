#pragma once
#include "IPapyrusClass.h"

class PapyrusSound final : public IPapyrusClass<PapyrusSound>
{
public:
  const char* GetName() override { return "Sound"; }

  VarValue Play(VarValue slef, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
