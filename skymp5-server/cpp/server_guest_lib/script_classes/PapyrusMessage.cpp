#include "PapyrusMessage.h"

void PapyrusMessage::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddMethod(vm, "Show", &PapyrusMessage::Show);
}
