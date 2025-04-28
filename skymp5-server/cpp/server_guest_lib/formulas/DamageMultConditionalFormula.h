#pragma once
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "IDamageFormula.h"
#include <nlohmann/json_fwd.hpp>

struct DamageMultConditionalFormulaSettingsValueCondition
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("function", function)
      .Serialize("runsOn", runsOn)
      .Serialize("comparison", comparison)
      .Serialize("value", value)
      .Serialize("parameter1", parameter1)
      .Serialize("logicalOperator", logicalOperator);
  }

  std::string function;
  std::string runsOn;
  std::string comparison; // ==, !=, >, <, >=, <=
  float value = 0.f;
  std::string parameter1;      // hex uint32_t
  std::string logicalOperator; // OR, AND
};

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

  std::vector<
    std::pair<std::string, DamageMultConditionalFormulaSettingsValue>>
    entries;
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

  bool EvaluateConditions(
    const std::vector<DamageMultConditionalFormulaSettingsValueCondition>&
      conditions,
    std::vector<int>* outConditionResolutions, const MpActor& aggressor,
    const MpActor& target) const;

  std::vector<std::string> LogEvaluateConditionsResolution(
    const std::vector<DamageMultConditionalFormulaSettingsValueCondition>&
      conditions,
    const std::vector<int>& conditionResolutions, bool finalResult) const;

  bool EvaluateCondition(
    const DamageMultConditionalFormulaSettingsValueCondition& condition,
    const MpActor& aggressor, const MpActor& target) const;

  bool CompareFloats(float a, float b, const std::string& op) const;

private:
  std::unique_ptr<IDamageFormula> baseFormula;
  std::optional<DamageMultConditionalFormulaSettings> settings;
};
