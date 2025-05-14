#pragma once
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "IDamageFormula.h"
#include <nlohmann/json_fwd.hpp>
#include "Condition.h"

struct DamageMultConditionalFormulaSettingsValue
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("physicalDamageMultiplier", physicalDamageMultiplier);
    archive.Serialize("magicDamageMultiplier", magicDamageMultiplier);
    archive.Serialize("conditions", conditions);
  }

  std::optional<float> physicalDamageMultiplier;
  std::optional<float> magicDamageMultiplier;
  std::vector<DamageMultConditionalFormulaSettingsValueCondition> conditions;
};

// TODO: add Serialize method
// TODO: support std::unordered_map support in JsonInputArchive
// TODO: support non-object json values in JsonInputArchive (like nic11 does
// for simdjson)
struct DamageMultConditionalFormulaSettings
{
  static DamageMultConditionalFormulaSettings FromJson(
    const nlohmann::json& j);

  std::vector<std::pair<std::string, Condition>> entries;
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
