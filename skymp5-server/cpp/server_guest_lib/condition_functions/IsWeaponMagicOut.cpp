#include "IsWeaponMagicOut.h"
#include "MpActor.h"

const char* ConditionFunctions::IsWeaponMagicOut::GetName() const
{
  return "IsWeaponMagicOut";
}

uint16_t ConditionFunctions::IsWeaponMagicOut::GetFunctionIndex() const
{
  return 101;
}

float ConditionFunctions::IsWeaponMagicOut::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext&)
{
  if (!actor.IsWeaponDrawn()) {
    return 0.f;
  }

  auto& equipment = actor.GetEquipment();

  if (equipment.leftSpell.has_value() && *equipment.leftSpell > 0) {
    return 1.f;
  }

  if (equipment.rightSpell.has_value() && *equipment.rightSpell > 0) {
    return 1.f;
  }

  auto& br = actor.GetParent()->GetEspm().GetBrowser();

  for (auto& entry : equipment.inv.entries) {
    if (entry.GetWorn() == Inventory::Worn::None) {
      continue;
    }

    auto lookupRes = br.LookupById(entry.baseId);
    if (!lookupRes.rec) {
      continue;
    }

    if (lookupRes.rec->GetType().ToString() != espm::WEAP::kType) {
      continue;
    }

    return 1.f;
  }

  return 0.f;
}
