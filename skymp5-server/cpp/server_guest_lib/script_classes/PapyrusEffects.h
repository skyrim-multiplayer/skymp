#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

class PapyrusEffects final : public IPapyrusClass<PapyrusEffects>
{
public:
  PapyrusEffects(const std::string& name)
    : strName(name)
  {
  }
  const char* GetName() override { return strName.c_str(); }
  VarValue Play(VarValue self, const std::vector<VarValue>& arguments);
  VarValue Stop(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    AddMethod(vm, "Play", &PapyrusEffects::Play);
    AddMethod(vm, "Stop", &PapyrusEffects::Stop);
  }

private:
  void Helper(VarValue& self, const char* funcName,
              const std::vector<VarValue>& arguments);

private:
  std::string strName;
};
