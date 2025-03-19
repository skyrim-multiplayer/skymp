#pragma once
#include "IPapyrusClass.h"

class PapyrusActor final : public IPapyrusClass<PapyrusActor>
{
public:
  const char* GetName() override { return "Actor"; }

  VarValue DrawWeapon(VarValue self, const std ::vector<VarValue>& arguments);
  VarValue UnequipAll(VarValue self, const std ::vector<VarValue>& arguments);
  VarValue PlayIdle(VarValue self, const std ::vector<VarValue>& arguments);
  VarValue GetSitState(VarValue self, const std ::vector<VarValue>& arguments);

  VarValue IsWeaponDrawn(VarValue self,
                         const std::vector<VarValue>& arguments);

  VarValue RestoreActorValue(VarValue self,
                             const std::vector<VarValue>& arguments);

  VarValue SetActorValue(VarValue self,
                         const std::vector<VarValue>& arguments);

  VarValue DamageActorValue(VarValue self,
                            const std::vector<VarValue>& arguments);

  VarValue IsEquipped(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetActorValuePercentage(VarValue self,
                                   const std::vector<VarValue>& arguments);

  VarValue SetAlpha(VarValue self, const std::vector<VarValue>& arguments);

  VarValue EquipItem(VarValue self, const std::vector<VarValue>& arguments);

  VarValue EquipSpell(VarValue self, const std::vector<VarValue>& arguments);

  VarValue UnequipItem(VarValue self, const std::vector<VarValue>& arguments);

  VarValue SetDontMove(VarValue self, const std::vector<VarValue>& arguments);

  VarValue IsDead(VarValue self,
                  const std::vector<VarValue>& arguments) const noexcept;

  VarValue WornHasKeyword(VarValue self,
                          const std::vector<VarValue>& arguments);

  VarValue AddToFaction(VarValue self, const std::vector<VarValue>& arguments);

  VarValue IsInFaction(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetFactions(VarValue self, const std::vector<VarValue>& arguments);

  VarValue RemoveFromFaction(VarValue self,
                             const std::vector<VarValue>& arguments);

  VarValue AddSpell(VarValue self, const std::vector<VarValue>& arguments);

  VarValue RemoveSpell(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetRace(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetSpellCount(VarValue self,
                         const std::vector<VarValue>& arguments);

  VarValue GetNthSpell(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;
};
