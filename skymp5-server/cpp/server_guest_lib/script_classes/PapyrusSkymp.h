#pragma once
#include "IPapyrusClass.h"
#include "WorldState.h"
#include "script_objects/EspmGameObject.h"

class PapyrusSkymp final : public IPapyrusClass<PapyrusSkymp>
{
public:
  const char* GetName() override { return "skymp"; }

  VarValue SetDefaultActor(VarValue self,
                           const std::vector<VarValue>& arguments);

  VarValue GetFaction(VarValue self, const std::vector<VarValue>& arguments);

  void Register(
    VirtualMachine& vm,
    std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy) override
  {
    policy = compatibilityPolicy;

    compatibilityPolicy = policy;

    factions = compatibilityPolicy->GetWorldState()
                 ->GetEspm()
                 .GetBrowser()
                 .GetRecordsByType("FACT");

    AddStatic(vm, "SetDefaultActor", &PapyrusSkymp::SetDefaultActor);
    AddStatic(vm, "GetFaction", &PapyrusSkymp::GetFaction);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> policy;
  std::vector<const std::vector<const espm::RecordHeader*>*> factions;
};
