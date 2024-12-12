#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusQuest final : public IPapyrusClass<PapyrusQuest>
{
public:
  const char* GetName() override { return "Quest"; }

  // Non-native in original Papyrus, just a wrapper around GetCurrentStageID
  VarValue GetStage(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetCurrentStageID(VarValue self,
                             const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
