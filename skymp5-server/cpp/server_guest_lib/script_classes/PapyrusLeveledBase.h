#pragma once
#include "IPapyrusClass.h"

class PapyrusLeveledBase : public IPapyrusClass<PapyrusLeveledBase>
{
public:
  PapyrusLeveledBase(const std::string& name)
    : strName(name)
  {
  }
  const char* GetName() override { return strName.c_str(); }

  VarValue GetNthForm(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;
private:
  std::string strName;
};
