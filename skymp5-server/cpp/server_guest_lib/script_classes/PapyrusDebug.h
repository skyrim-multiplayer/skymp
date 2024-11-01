#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusDebug final : public IPapyrusClass<PapyrusDebug>
{
public:
  const char* GetName() override { return "Debug"; }

  DEFINE_STATIC_SPSNIPPET(Notification);
  DEFINE_STATIC_SPSNIPPET(MessageBox);

  VarValue SendAnimationEvent(VarValue self,
                              const std::vector<VarValue>& arguments);

  VarValue Trace(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
