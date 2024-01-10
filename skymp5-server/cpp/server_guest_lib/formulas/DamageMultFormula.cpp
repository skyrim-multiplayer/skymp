#include "DamageMultFormula.h"

#include "MpActor.h"

namespace {
bool IsNonPlayerBaseId(const MpActor& actor)
{
  return actor.GetBaseId() != 0x7;
}
}

DamageMultFormula::DamageMultFormula(
  std::unique_ptr<IDamageFormula> baseFormula_)
  : baseFormula(std::move(baseFormula_))
{
}

float DamageMultFormula::CalculateDamage(const MpActor& aggressor,
                                         const MpActor& target,
                                         const HitData& hitData) const
{
  float baseDamage = baseFormula->CalculateDamage(aggressor, target, hitData);

  auto worldState = aggressor.GetParent();
  if (!worldState) {
    return baseDamage;
  }

  float fCombatDistance =
    espm::GetData<espm::GMST>(espm::GMST::kFCombatDistance, worldState).value;

  if (IsNonPlayerBaseId(aggressor) && !IsNonPlayerBaseId(target)) {
    baseDamage *= 2.0f;
  }

  return baseDamage;
}
