#pragma once
#include "IPapyrusClass.h"

class PapyrusMessage final : public IPapyrusClass<PapyrusMessage>
{
public:
  const char* GetName() override { return "Message"; }

  VarValue Show(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;
};
