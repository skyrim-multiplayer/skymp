#pragma once
#include "EspmGameObject.h"
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusCell final : public IPapyrusClass<PapyrusCell>
{
public:
  const char* GetName() override { return "cell"; }

  VarValue IsAttached(VarValue self,
                         const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
