#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusEffectShader final : public IPapyrusClass<PapyrusEffectShader>
{
public:
  const char* GetName() override { return "EffectShader"; }
  VarValue Play(VarValue self, const std::vector<VarValue>& arguments);
  VarValue Stop(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    AddMethod(vm, "play", &PapyrusEffectShader::Play);
    AddMethod(vm, "stop", &PapyrusEffectShader::Stop);
  }

private:
  void Helper(VarValue& self, const char* funcName,
              const std::vector<VarValue>& arguments);
};
