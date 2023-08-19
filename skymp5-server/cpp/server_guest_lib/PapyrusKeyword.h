#pragma once
#include "EspmGameObject.h"
#include "IPapyrusClass.h"
#include "WorldState.h"

class PapyrusKeyword final : public IPapyrusClass<PapyrusKeyword>
{
public:
  const char* GetName() override { return "keyword"; }

  VarValue GetKeyword(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    compatibilityPolicy = policy;

    keywords = compatibilityPolicy->GetWorldState()
                 ->GetEspm()
                 .GetBrowser()
                 .GetRecordsByType("KYWD");

    AddStatic(vm, "GetKeyword", &PapyrusKeyword::GetKeyword);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
  std::vector<const std::vector<const espm::RecordHeader*>*> keywords;
};
