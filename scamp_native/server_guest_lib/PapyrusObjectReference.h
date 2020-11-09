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
  VarValue GetAnimationVariableBool(VarValue self,
                                    const std::vector<VarValue>& arguments);
  VarValue PlaceAtMe(VarValue self, const std::vector<VarValue>& arguments);
  VarValue SetAngle(VarValue self, const std::vector<VarValue>& arguments);
  VarValue Disable(VarValue self, const std::vector<VarValue>& arguments);
  VarValue BlockActivation(VarValue self,
                           const std::vector<VarValue>& arguments);
  VarValue IsActivationBlocked(VarValue self,
                               const std::vector<VarValue>& arguments);
  VarValue Activate(VarValue self, const std::vector<VarValue>& arguments);

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
    AddMethod(vm, "GetAnimationVariableBool",
              &PapyrusObjectReference::GetAnimationVariableBool);
    AddMethod(vm, "PlaceAtMe", &PapyrusObjectReference::PlaceAtMe);
    AddMethod(vm, "SetAngle", &PapyrusObjectReference::SetAngle);
    AddMethod(vm, "Disable", &PapyrusObjectReference::Disable);
    AddMethod(vm, "BlockActivation", &PapyrusObjectReference::BlockActivation);
    AddMethod(vm, "IsActivationBlocked",
              &PapyrusObjectReference::IsActivationBlocked);
    AddMethod(vm, "Activate", &PapyrusObjectReference::Activate);
  }
};