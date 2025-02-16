#pragma once
#include "IPapyrusClass.h"

class PapyrusDebug final : public IPapyrusClass<PapyrusDebug>
{
public:
  const char* GetName() override { return "Debug"; }

  VarValue Notification(VarValue self,
                        const std ::vector<VarValue>& arguments);
  VarValue MessageBox(VarValue self, const std ::vector<VarValue>& arguments);

  VarValue SendAnimationEvent(VarValue self,
                              const std::vector<VarValue>& arguments);

  VarValue Trace(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;
};
