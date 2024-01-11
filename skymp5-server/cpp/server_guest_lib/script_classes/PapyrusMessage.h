#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusMessage final : public IPapyrusClass<PapyrusMessage>
{
public:
  const char* GetName() override { return "message"; }

  DEFINE_METHOD_SPSNIPPET(Show);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
