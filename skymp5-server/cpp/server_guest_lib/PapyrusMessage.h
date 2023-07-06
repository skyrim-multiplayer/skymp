#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusMessage : public IPapyrusClass<PapyrusMessage>
{
public:
  const char* GetName() override { return "message"; }

  DEFINE_METHOD_SPSNIPPET(Show);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy,
                WorldState* world) override
  {
    compatibilityPolicy = policy;

    AddMethod(vm, "Show", &PapyrusMessage::Show);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
