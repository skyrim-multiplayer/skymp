#include "DamageMultConditionalFormula.h"

DamageMultConditionalFormula::DamageMultConditionalFormula(
  std::unique_ptr<IDamageFormula> baseFormula_, const nlohmann::json& config)
  : baseFormula(std::move(baseFormula_))
  , settings(std::nullopt)
{
  if (config.is_object()) {
    settings = ParseConfig(config);
  }
}

float DamageMultConditionalFormula::CalculateDamage(
  const MpActor& aggressor, const MpActor& target,
  const HitData& hitData) const
{
}

float DamageMultConditionalFormula::CalculateDamage(
  const MpActor& aggressor, const MpActor& target,
  const SpellCastData& spellCastData) const
{
}

DamageMultConditionalFormulaSettings DamageMultConditionalFormula::ParseConfig(
  const nlohmann::json& config) const
{
  return SweetPieSpellDamageFormulaSettings::FromJson(config);
}

DamageMultConditionalFormulaSettings
DamageMultConditionalFormulaSettings::FromJson(const nlohmann::json& j)
{
  JsonInputArchive ar(j);
  DamageMultConditionalFormulaSettings res;
  res.Serialize(ar);
  return res;
}