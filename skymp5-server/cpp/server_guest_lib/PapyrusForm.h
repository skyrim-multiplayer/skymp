#pragma once
#include "IPapyrusClass.h"

class PapyrusForm : public IPapyrusClass<PapyrusForm>
{
public:
  const char* GetName() override { return "form"; }

  VarValue RegisterForSingleUpdate(VarValue self,
                                   const std::vector<VarValue>& arguments);

  VarValue GetType(VarValue self, const std::vector<VarValue>& arguments);

  VarValue HasKeyword(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy,
                WorldState* world) override
  {
    AddMethod(vm, "RegisterForSingleUpdate",
              &PapyrusForm::RegisterForSingleUpdate);
    AddMethod(vm, "GetType", &PapyrusForm::GetType);
    AddMethod(vm, "HasKeyword", &PapyrusForm::HasKeyword);
  }
};
