#pragma once
#include "IPapyrusClass.h"
#include <memory>
#include <vector>

class IPapyrusCompatibilityPolicy;
class VirtualMachine;

class PapyrusClassesFactory
{
public:
  static std::vector<std::unique_ptr<IPapyrusClassBase>> CreateAndRegister(
    VirtualMachine& vm,
    const std::shared_ptr<IPapyrusCompatibilityPolicy>& compatibilityPolicy);
};
