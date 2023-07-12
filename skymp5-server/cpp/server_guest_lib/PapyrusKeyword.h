#pragma once
#include "EspmGameObject.h"
#include "IPapyrusClass.h"
#include "WorldState.h"

class PapyrusKeyword : public IPapyrusClass<PapyrusKeyword>
{
public:
  const char* GetName() override { return "keyword"; }

  VarValue GetKeyword(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy,
                WorldState* world) override
  {
    compatibilityPolicy = policy;
    worldState = world;

    keywords = world->GetEspm().GetBrowser().GetRecordsByType("KYWD");

    AddStatic(vm, "GetKeyword", &PapyrusKeyword::GetKeyword);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
  WorldState* worldState;
  std::vector<const std::vector<espm::RecordHeader*>*> keywords;
};
