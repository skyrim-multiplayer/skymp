#pragma once
#include "IPapyrusClass.h"

class PapyrusEffectBase : public IPapyrusClass<PapyrusEffectBase>
{
public:
  PapyrusEffectBase(const std::string& name)
    : strName(name)
  {
  }
  const char* GetName() override { return strName.c_str(); }
  VarValue Play(VarValue self, const std::vector<VarValue>& arguments);
  VarValue Stop(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

private:
  void Helper(VarValue& self, const char* funcName,
              const std::vector<VarValue>& arguments);

private:
  std::string strName;
};
