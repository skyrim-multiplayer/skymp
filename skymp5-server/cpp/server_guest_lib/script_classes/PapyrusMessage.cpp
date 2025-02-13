#include "PapyrusMessage.h"

VarValue PapyrusMessage::Show(VarValue self,
                              const std::vector<VarValue>& arguments)
{
  return MakeSPSnippetPromise(GetName(), "Show", compatibilityPolicy, self,
                              arguments, true, true);
}

void PapyrusMessage::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddMethod(vm, "Show", &PapyrusMessage::Show);
}
