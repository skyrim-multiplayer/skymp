#pragma once
#include "VirtualMachine.h"
#include <vector>

namespace PapyrusObjectReference {
VarValue IsDisabled(VarValue self, const std::vector<VarValue>& arguments);
VarValue GetScale(VarValue self, const std::vector<VarValue>& arguments);
VarValue SetScale(VarValue self, const std::vector<VarValue>& arguments);
VarValue EnableNoWait(VarValue self, const std::vector<VarValue>& arguments);
VarValue DisableNoWait(VarValue self, const std::vector<VarValue>& arguments);
VarValue Delete(VarValue self, const std::vector<VarValue>& arguments);
VarValue AddItem(VarValue self, const std::vector<VarValue>& arguments);

inline void Register(VirtualMachine& vm)
{
  vm.RegisterFunction("objectreference", "IsDisabled", FunctionType::Method,
                      IsDisabled);
  vm.RegisterFunction("objectreference", "GetScale", FunctionType::Method,
                      GetScale);
  vm.RegisterFunction("objectreference", "SetScale", FunctionType::Method,
                      SetScale);
  vm.RegisterFunction("objectreference", "EnableNoWait", FunctionType::Method,
                      EnableNoWait);
  vm.RegisterFunction("objectreference", "DisableNoWait", FunctionType::Method,
                      DisableNoWait);
  vm.RegisterFunction("objectreference", "Delete", FunctionType::Method,
                      Delete);
  vm.RegisterFunction("objectreference", "AddItem", FunctionType::Method,
                      AddItem);
}
}