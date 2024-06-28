#pragma once
#include "IPapyrusClass.h"
#include "WorldState.h"
#include "script_objects/EspmGameObject.h"

class PapyrusFaction final : public IPapyrusClass<PapyrusFaction>
{
public:
  const char* GetName() override { return "faction"; }

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

    AddMethod(vm, "GetReaction", &PapyrusFaction::GetReaction);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
  std::vector<const std::vector<const espm::RecordHeader*>*> factions;
};
