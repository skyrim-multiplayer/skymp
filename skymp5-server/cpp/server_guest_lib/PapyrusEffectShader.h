#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusEffectShader : public IPapyrusClass<PapyrusEffectShader>
{
public:
  const char* GetName() override { return "effectshader"; }
  VarValue Play(VarValue self, const std::vector<VarValue>& arguments);
  VarValue Stop(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy,
                WorldState* world) override
  {
    AddMethod(vm, "Play", &PapyrusEffectShader::Play);
    AddMethod(vm, "Stop", &PapyrusEffectShader::Stop);
  }

private:
  void Helper(VarValue& self, const char* funcName,
              const std::vector<VarValue>& arguments);
};
