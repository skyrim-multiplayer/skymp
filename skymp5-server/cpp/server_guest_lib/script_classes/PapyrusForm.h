#pragma once
#include "IPapyrusClass.h"

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

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    AddMethod(vm, "registerForSingleUpdate",
              &PapyrusForm::RegisterForSingleUpdate);
    AddMethod(vm, "getType", &PapyrusForm::GetType);
    AddMethod(vm, "hasKeyword", &PapyrusForm::HasKeyword);
    AddMethod(vm, "getFormID", &PapyrusForm::GetFormId);
    AddMethod(vm, "getName", &PapyrusForm::GetName_);

    this->compatibilityPolicy = policy;
  }

private:
  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
