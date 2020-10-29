#pragma once
#include "IPapyrusClass.h"

class PapyrusForm : public IPapyrusClass<PapyrusForm>
{
public:
  const char* GetName() override { return "form"; }

  VarValue RegisterForSingleUpdate(VarValue self,
                                   const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy>) override
  {
    AddMethod(vm, "RegisterForSingleUpdate",
              &PapyrusForm::RegisterForSingleUpdate);
  }
};