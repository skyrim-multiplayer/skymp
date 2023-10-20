#include "PapyrusCell.h"

VarValue PapyrusCell::IsAttached(VarValue self,
                                 const std::vector<VarValue>& arguments)
{
  return VarValue(true); // stub
}

void PapyrusCell::Register(VirtualMachine& vm,
                           std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddMethod(vm, "IsAttached", &PapyrusCell::IsAttached);
}
