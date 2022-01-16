#pragma once
#include "IPapyrusClass.h"

#include "SpSnippetFunctionGen.h"

class PapyrusActor : public IPapyrusClass<PapyrusActor>
{
public:
  const char* GetName() override { return "actor"; }

  DEFINE_METHOD_SPSNIPPET(DrawWeapon);
  DEFINE_METHOD_SPSNIPPET(UnequipAll);
  DEFINE_METHOD_SPSNIPPET(PlayIdle);
  DEFINE_METHOD_SPSNIPPET(GetSitState);

  VarValue IsWeaponDrawn(VarValue self,
                         const std::vector<VarValue>& arguments);

  VarValue RestoreActorValue(VarValue self,
                             const std::vector<VarValue>& arguments);

  VarValue DamageActorValue(VarValue self,
                            const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override
  {
    compatibilityPolicy = policy;

    AddMethod(vm, "IsWeaponDrawn", &PapyrusActor::IsWeaponDrawn);
    AddMethod(vm, "DrawWeapon", &PapyrusActor::DrawWeapon);
    AddMethod(vm, "UnequipAll", &PapyrusActor::UnequipAll);
    AddMethod(vm, "PlayIdle", &PapyrusActor::PlayIdle);
    AddMethod(vm, "GetSitState", &PapyrusActor::GetSitState);
    AddMethod(vm, "RestoreActorValue", &PapyrusActor::RestoreActorValue);
    AddMethod(vm, "DamageActorValue", &PapyrusActor::DamageActorValue);
  }

  std::shared_ptr<IPapyrusCompatibilityPolicy> compatibilityPolicy;
};
