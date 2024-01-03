#pragma once
#include "IPapyrusClass.h"

class PapyrusObjectReference final
  : public IPapyrusClass<PapyrusObjectReference>
{
public:
  const char* GetName() override { return "objectreference"; }

  VarValue IsHarvested(VarValue self, const std::vector<VarValue>& arguments);
  VarValue IsDisabled(VarValue self, const std::vector<VarValue>& arguments);
  VarValue IsDeleted(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetScale(VarValue self, const std::vector<VarValue>& arguments);
  VarValue SetScale(VarValue self, const std::vector<VarValue>& arguments);
  VarValue EnableNoWait(VarValue self, const std::vector<VarValue>& arguments);
  VarValue DisableNoWait(VarValue self,
                         const std::vector<VarValue>& arguments);
  VarValue AddItem(VarValue self, const std::vector<VarValue>& arguments);
  VarValue RemoveItem(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetItemCount(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetAnimationVariableBool(VarValue self,
                                    const std::vector<VarValue>& arguments);
  VarValue PlaceAtMe(VarValue self, const std::vector<VarValue>& arguments);
  VarValue SetAngle(VarValue self, const std::vector<VarValue>& arguments);
  VarValue Enable(VarValue self, const std::vector<VarValue>& arguments);
  VarValue Disable(VarValue self, const std::vector<VarValue>& arguments);
  VarValue Delete(VarValue self, const std::vector<VarValue>& arguments);
  VarValue BlockActivation(VarValue self,
                           const std::vector<VarValue>& arguments);
  VarValue IsActivationBlocked(VarValue self,
                               const std::vector<VarValue>& arguments);
  VarValue Activate(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetPositionX(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetPositionY(VarValue self, const std::vector<VarValue>& arguments);
  VarValue GetPositionZ(VarValue self, const std::vector<VarValue>& arguments);
  VarValue SetPosition(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetBaseObject(VarValue self,
                         const std::vector<VarValue>& arguments);
  VarValue PlayAnimation(VarValue self,
                         const std::vector<VarValue>& arguments);
  VarValue PlayAnimationAndWait(VarValue self,
                                const std::vector<VarValue>& arguments);
  VarValue PlayGamebryoAnimation(VarValue self,
                                 const std::vector<VarValue>& arguments);
  VarValue MoveTo(VarValue self, const std::vector<VarValue>& arguments);

  VarValue SetOpen(VarValue self, const std::vector<VarValue>& arguments);

  VarValue Is3DLoaded(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetLinkedRef(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetNthLinkedRef(VarValue self,
                           const std::vector<VarValue>& arguments);

  VarValue GetParentCell(VarValue self,
                         const std::vector<VarValue>& arguments);

  VarValue GetOpenState(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetAllItemsCount(VarValue self,
                            const std::vector<VarValue>& arguments);

  VarValue IsContainerEmpty(VarValue self,
                            const std::vector<VarValue>& arguments);

  VarValue SetDisplayName(VarValue self,
                          const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;
};
