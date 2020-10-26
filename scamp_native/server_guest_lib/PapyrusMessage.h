#pragma once
#include "IPapyrusClass.h"

class PapyrusMessage : public IPapyrusClass<PapyrusMessage>
{
public:
  const char* GetName() override { return "message"; }

  VarValue Show(VarValue self, const std::vector<VarValue>& arguments)
  {
    assert(0 && "SHOW!!!");
    return VarValue::None();
  }

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy>) override
  {
    AddMethod(vm, "Show", &PapyrusMessage::Show);
  }
};