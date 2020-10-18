#pragma once
#include "IPapyrusClass.h"

class PapyrusObjectReference : public IPapyrusClass<PapyrusObjectReference>
{
public:
  const char* GetName() override { return "objectreference"; }

  VarValue IsDisabled(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetScale(VarValue self, const std::vector<VarValue>& arguments);
  VarValue SetScale(VarValue self, const std::vector<VarValue>& arguments);
  VarValue EnableNoWait(VarValue self, const std::vector<VarValue>& arguments);
  VarValue DisableNoWait(VarValue self,
                         const std::vector<VarValue>& arguments);
  VarValue Delete(VarValue self, const std::vector<VarValue>& arguments);
  VarValue AddItem(VarValue self, const std::vector<VarValue>& arguments);
  VarValue RemoveItem(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetItemCount(VarValue self, const std::vector<VarValue>& arguments);

  void Register(
    VirtualMachine& vm,
    std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy) override
  {
    AddMethod(vm, "IsDisabled", &PapyrusObjectReference::IsDisabled);
    AddMethod(vm, "GetScale", &PapyrusObjectReference::GetScale);
    AddMethod(vm, "SetScale", &PapyrusObjectReference::SetScale);
    AddMethod(vm, "EnableNoWait", &PapyrusObjectReference::EnableNoWait);
    AddMethod(vm, "DisableNoWait", &PapyrusObjectReference::DisableNoWait);
    AddMethod(vm, "Delete", &PapyrusObjectReference::Delete);
    AddMethod(vm, "AddItem", &PapyrusObjectReference::AddItem);
    AddMethod(vm, "RemoveItem", &PapyrusObjectReference::RemoveItem);
    AddMethod(vm, "GetItemCount", &PapyrusObjectReference::GetItemCount);
  }
};