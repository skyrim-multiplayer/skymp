#pragma once
#include "IPapyrusClass.h"

class PapyrusFormList final : public IPapyrusClass<PapyrusFormList>
{
public:
  const char* GetName() override { return "FormList"; }

  VarValue GetSize(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetAt(VarValue self, const std::vector<VarValue>& arguments);
  VarValue Find(VarValue self, const std::vector<VarValue>& arguments) const;

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    AddMethod(vm, "getSize", &PapyrusFormList::GetSize);
    AddMethod(vm, "getAt", &PapyrusFormList::GetAt);
    AddMethod(vm, "find", &PapyrusFormList::Find);
  }
};
