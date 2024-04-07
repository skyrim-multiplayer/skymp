#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"
#include "script_objects/EspmGameObject.h"

class PapyrusCell final : public IPapyrusClass<PapyrusCell>
{
public:
  const char* GetName() override { return "Cell"; }

  VarValue IsAttached(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
