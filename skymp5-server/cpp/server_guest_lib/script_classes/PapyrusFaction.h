#pragma once
#include "IPapyrusClass.h"
#include "WorldState.h"
#include "script_objects/EspmGameObject.h"

class PapyrusFaction final : public IPapyrusClass<PapyrusFaction>
{
public:
  const char* GetName() override { return "faction"; }

  // NOT VANILLA/SKSE ONE. Custom. (looks as Keyword.GetKeyword)
  VarValue GetFaction(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetReaction(VarValue self, const std::vector<VarValue>& arguments);
  // SetReaction ignored, because no way to edit factions forever?

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    compatibilityPolicy = policy;

    factions = compatibilityPolicy->GetWorldState()
                 ->GetEspm()
                 .GetBrowser()
                 .GetRecordsByType("FACT");

    AddStatic(vm, "GetFaction", &PapyrusFaction::GetFaction);

    AddMethod(vm, "GetReaction", &PapyrusFaction::GetFaction);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
  std::vector<const std::vector<const espm::RecordHeader*>*> factions;
};
