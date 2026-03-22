#include "IsWeaponOut.h"
#include "MpActor.h"

const char* ConditionFunctions::IsWeaponOut::GetName() const
{
  return "IsWeaponOut";
}

uint16_t ConditionFunctions::IsWeaponOut::GetFunctionIndex() const
{
  return 263;
}

float ConditionFunctions::IsWeaponOut::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext&)
{
  if (!actor.IsWeaponDrawn()) {
    return 0.f;
  }

  auto& br = actor.GetParent()->GetEspm().GetBrowser();

  for (auto& entry : actor.GetEquipment().inv.entries) {
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

    return 2.f;
  }

  return 1.f;
}
