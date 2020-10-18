#pragma once
#include "IPapyrusClass.h"

class PapyrusGame : public IPapyrusClass<PapyrusGame>
{
  const char* GetName() override { return "game"; }

  VarValue IncrementStat(VarValue self,
                         const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy>) override
  {
    AddStatic(vm, "IncrementStat", &PapyrusGame::IncrementStat);
  }
};