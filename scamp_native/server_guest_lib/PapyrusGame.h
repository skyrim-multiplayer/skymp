#pragma once
#include "VirtualMachine.h"
#include <vector>

namespace PapyrusGame {
VarValue IncrementStat(VarValue self, const std::vector<VarValue>& arguments);
VarValue RegisterForSingleUpdate(VarValue self,
                                 const std::vector<VarValue>& arguments);

inline void Register(VirtualMachine& vm)
{
  vm.RegisterFunction("game", "IncrementStat", FunctionType::GlobalFunction,
                      IncrementStat);
  vm.RegisterFunction("game", "RegisterForSingleUpdate",
                      FunctionType::GlobalFunction, RegisterForSingleUpdate);
}

}