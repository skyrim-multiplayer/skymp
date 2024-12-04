#include "SweetPieSpellDamageFormula.h"

#include "archives/JsonInputArchive.h"
#include <limits>

namespace SweetPieSpellDamageFormulaPrivate {
template <class T>
T Clamp(T value, T minValue, T maxValue)
{
  if (value < minValue) {
    return minValue;
  } else if (value > maxValue) {
    return maxValue;
  } else {
    return value;
  }
}
}

SweetPieSpellDamageFormulaSettings
SweetPieSpellDamageFormulaSettings::FromJson(const nlohmann::json& j)
{
  JsonInputArchive ar(j);
  SweetPieSpellDamageFormulaSettings res;
  res.Serialize(ar);
  return res;
}

SweetPieSpellDamageFormula::SweetPieSpellDamageFormula(
  std::unique_ptr<IDamageFormula> baseFormula_, const nlohmann::json& config)
  : baseFormula(std::move(baseFormula_))
  , settings(std::nullopt)
{
  if (config.is_object()) {
    settings = ParseConfig(config);
  }
}

float SweetPieSpellDamageFormula::CalculateDamage(
  const MpActor& aggressor, const MpActor& target,
  const HitData& hitData) const override
{
  return baseFormula->CalculateDamage(aggressor, target, hitData);
}

float SweetPieSpellDamageFormula::CalculateDamage(
  const MpActor& aggressor, const MpActor& target,
  const SpellCastData& spellCastData) const override
{
  const float baseDamage =
    baseFormula->CalculateDamage(aggressor, target, spellCastData);

  if (!settings) {
    return baseDamage;
  }

  float biggestMult = -1;

  for (auto& entry : settings->entries) {
    const auto itemId = entry.itemId;
    const auto mult = SweetPieSpellDamageFormulaPrivate::Clamp(
      entry.mult, 0.f, std::numeric_limits<float>::infinity());

    if (aggressor.GetItemCount(itemId) > 0) {
      biggestMult = std::max(biggestMult, mult);
    }
  }

  return biggestMult >= 0 ? biggestMult * baseDamage : baseDamage;
}

SweetPieSpellDamageFormulaSettings SweetPieSpellDamageFormula::ParseConfig(
  const nlohmann::json& config) const
{
  return SweetPieSpellDamageFormulaSettings::FromJson(config);
}
