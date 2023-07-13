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
  VarValue FindClosestReferenceOfAnyTypeInListFromRef(
    VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetPlayer(VarValue self, const std::vector<VarValue>& arguments);
  VarValue ShowRaceMenu(VarValue self, const std::vector<VarValue>& arguments);
  VarValue ShowLimitedRaceMenu(VarValue self,
                               const std::vector<VarValue>& arguments);
  VarValue GetCameraState(VarValue self,
                          const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy,
                WorldState* world) override
  {
    compatibilityPolicy = policy;

    AddStatic(vm, "IncrementStat", &PapyrusGame::IncrementStat);
    AddStatic(vm, "ForceThirdPerson", &PapyrusGame::ForceThirdPerson);
    AddStatic(vm, "DisablePlayerControls",
              &PapyrusGame::DisablePlayerControls);
    AddStatic(vm, "EnablePlayerControls", &PapyrusGame::EnablePlayerControls);
    AddStatic(vm, "FindClosestReferenceOfAnyTypeInListFromRef",
              &PapyrusGame::FindClosestReferenceOfAnyTypeInListFromRef);
    AddStatic(vm, "GetPlayer", &PapyrusGame::GetPlayer);
    AddStatic(vm, "ShowRaceMenu", &PapyrusGame::ShowRaceMenu);
    AddStatic(vm, "ShowLimitedRaceMenu", &PapyrusGame::ShowLimitedRaceMenu);
    AddStatic(vm, "GetCameraState", &PapyrusGame::GetCameraState);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;

private:
  void RaceMenuHelper(VarValue& self, const char* funcName,
                      const std::vector<VarValue>& arguments);
};
