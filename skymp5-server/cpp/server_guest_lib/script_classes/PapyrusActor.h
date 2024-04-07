#pragma once
#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"
#include "script_objects/EspmGameObject.h"

class PapyrusActor final : public IPapyrusClass<PapyrusActor>
{
public:
  const char* GetName() override { return "Actor"; }

  DEFINE_METHOD_SPSNIPPET(DrawWeapon);
  DEFINE_METHOD_SPSNIPPET(UnequipAll);
  DEFINE_METHOD_SPSNIPPET(PlayIdle);
  DEFINE_METHOD_SPSNIPPET(GetSitState);

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

  VarValue UnequipItem(VarValue self, const std::vector<VarValue>& arguments);

  VarValue SetDontMove(VarValue self, const std::vector<VarValue>& arguments);

  VarValue IsDead(VarValue self,
                  const std::vector<VarValue>& arguments) const noexcept;

  VarValue WornHasKeyword(VarValue self,
                          const std::vector<VarValue>& arguments);

  VarValue AddSpell(VarValue self, const std::vector<VarValue>& arguments);

  VarValue RemoveSpell(VarValue self, const std::vector<VarValue>& arguments);

  VarValue GetRace(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
