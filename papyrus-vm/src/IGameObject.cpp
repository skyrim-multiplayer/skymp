#include "papyrus-vm/Structures.h"
#include "papyrus-vm/Utils.h"

bool IGameObject::HasScript(const char* scriptName) const
{
  for (auto& instance : activePexInstances) {
    const std::string& sourcePexName = instance->GetSourcePexName();
    if (!Utils::stricmp(sourcePexName.data(), scriptName)) {
      return true;
    }
  }
  return false;
}
