#pragma once
#include "IPapyrusClass.h"

class PapyrusVisualEffect final : public IPapyrusClass<PapyrusVisualEffect>
{
public:
  const char* GetName() override { return "visualeffect"; }
  VarValue Play(VarValue self, const std::vector<VarValue>& arguments);
  VarValue Stop(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    AddMethod(vm, "Play", &PapyrusVisualEffect::Play);
    AddMethod(vm, "Stop", &PapyrusVisualEffect::Stop);
  }

private:
  void Helper(VarValue& self, const char* funcName,
              const std::vector<VarValue>& arguments);
};
