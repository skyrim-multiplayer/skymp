#pragma once
#include <memory>

#include "IDamageFormula.h"
#include <nlohmann/json_fwd.hpp>

struct DamageMultConditionalFormulaSettingsEntry
{
  std::vector<Condition> conditionsList;
};

struct DamageMultConditionalFormulaSettings
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("entries", entries);
  }

  static DamageMultConditionalFormulaSettings FromJson(
    const nlohmann::json& j);

  std::vector<DamageMultConditionalFormulaSettingsEntry> entries;
};

class DamageMultConditionalFormula : public IDamageFormula
{
public:
  DamageMultConditionalFormula(std::unique_ptr<IDamageFormula> baseFormula_,
                               const nlohmann::json& config);

  [[nodiscard]] float CalculateDamage(const MpActor& aggressor,
                                      const MpActor& target,
                                      const HitData& hitData) const override;

  [[nodiscard]] float CalculateDamage(
    const MpActor& aggressor, const MpActor& target,
    const SpellCastData& spellCastData) const override;

private:
  DamageMultConditionalFormulaSettings ParseConfig(
    const nlohmann::json& config) const;

private:
  std::unique_ptr<IDamageFormula> baseFormula;
  std::optional<DamageMultConditionalFormulaSettings> settings;
};
