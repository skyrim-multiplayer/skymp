#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusGame final : public IPapyrusClass<PapyrusGame>
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
  VarValue GetForm(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetFormEx(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

public:
  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
  WorldState* worldState;

private:
  void RaceMenuHelper(VarValue& self, const char* funcName,
                      const std::vector<VarValue>& arguments);
};
