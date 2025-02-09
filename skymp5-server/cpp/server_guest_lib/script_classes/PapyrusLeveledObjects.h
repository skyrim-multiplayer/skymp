#pragma once
#include "IPapyrusClass.h"

class PapyrusLeveledObjects final : public IPapyrusClass<PapyrusLeveledObjects>
{
public:
  PapyrusLeveledObjects(const std::string& name)
    : strName(name)
  {
  }
  const char* GetName() override { return strName.c_str(); }

  VarValue GetNthForm(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;

private:
  std::string strName;
};
