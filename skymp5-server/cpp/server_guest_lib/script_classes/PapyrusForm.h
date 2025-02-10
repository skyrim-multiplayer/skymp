#pragma once
#include "IPapyrusClass.h"
#include "papyrus-vm/Structures.h"

class PapyrusForm final : public IPapyrusClass<PapyrusForm>
{
public:
  const char* GetName() override { return "Form"; }

  VarValue RegisterForSingleUpdate(VarValue self,
                                   const std::vector<VarValue>& arguments);

  VarValue GetType(VarValue self, const std::vector<VarValue>& arguments);

  VarValue HasKeyword(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetFormId(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetName_(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetWeight(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

private:
  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
