#pragma once
#include "IPapyrusClass.h"

class PapyrusDebug : public IPapyrusClass<PapyrusDebug>
{
public:
  const char* GetName() { return "debug"; }

  VarValue Notification(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    compatibilityPolicy = policy;

    AddStatic(vm, "Notification", &PapyrusDebug::Notification);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};