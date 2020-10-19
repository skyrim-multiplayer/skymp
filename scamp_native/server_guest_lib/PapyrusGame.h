#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusGame : public IPapyrusClass<PapyrusGame>
{
public:
  const char* GetName() override { return "game"; }

  DEFINE_STATIC_SPSNIPPET(ForceThirdPerson);
  DEFINE_STATIC_SPSNIPPET(DisablePlayerControls);
  DEFINE_STATIC_SPSNIPPET(EnablePlayerControls);

  VarValue IncrementStat(VarValue self,
                         const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    compatibilityPolicy = policy;

    AddStatic(vm, "IncrementStat", &PapyrusGame::IncrementStat);
    AddStatic(vm, "ForceThirdPerson", &PapyrusGame::ForceThirdPerson);
    AddStatic(vm, "DisablePlayerControls",
              &PapyrusGame::DisablePlayerControls);
    AddStatic(vm, "EnablePlayerControls", &PapyrusGame::EnablePlayerControls);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};