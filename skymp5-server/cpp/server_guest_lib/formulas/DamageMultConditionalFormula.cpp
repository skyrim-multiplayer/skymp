#include "DamageMultConditionalFormula.h"

#include "MpActor.h"
#include "archives/JsonInputArchive.h"
#include "papyrus-vm/Utils.h"
#include <fmt/fmt.h>
#include <functional>
#include <limits>
#include <sstream>

namespace {
uint32_t ExtractParameter(const std::string& parameter1)
{
  uint32_t parameter1Parsed = 0;

  if (parameter1.find("0x") == 0 || parameter1.find("0X") == 0) {
    std::stringstream ss;
    ss << std::hex << parameter1.substr(2); // Skip "0x"
    ss >> parameter1Parsed;
  } else {
    std::stringstream ss(parameter1);
    ss >> parameter1Parsed;
  }

  return parameter1Parsed;
}
}

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
  float baseDamage = baseFormula->CalculateDamage(aggressor, target, hitData);

  if (!settings) {
    return baseDamage;
  }

  for (auto [key, value] : settings->entries) {
    if (value.physicalDamageMultiplier.has_value()) {
      if (EvaluateConditions(value.conditions, aggressor, target)) {
        baseDamage *= *value.physicalDamageMultiplier;
      }
    }
  }

  return baseDamage;
}

float DamageMultConditionalFormula::CalculateDamage(
  const MpActor& aggressor, const MpActor& target,
  const SpellCastData& spellCastData) const
{
  float baseDamage =
    baseFormula->CalculateDamage(aggressor, target, spellCastData);

  if (!settings) {
    return baseDamage;
  }

  for (auto [key, value] : settings->entries) {
    if (value.magicDamageMultiplier.has_value()) {
      if (EvaluateConditions(value.conditions, aggressor, target)) {
        baseDamage *= *value.magicDamageMultiplier;
      }
    }
  }

  return baseDamage;
}

DamageMultConditionalFormulaSettings DamageMultConditionalFormula::ParseConfig(
  const nlohmann::json& config) const
{
  return DamageMultConditionalFormulaSettings::FromJson(config);
}

DamageMultConditionalFormulaSettings
DamageMultConditionalFormulaSettings::FromJson(const nlohmann::json& j)
{
  DamageMultConditionalFormulaSettings res;

  // iterate object
  for (auto it = j.begin(); it != j.end(); ++it) {
    const auto& key = it.key();
    const auto& value = it.value();

    JsonInputArchive ar(j);
    DamageMultConditionalFormulaSettingsValue valueParsed;
    valueParsed.Serialize(ar);

    res.entries.emplace_back(key, valueParsed);
  }

  // validate
  for (auto& [key, value] : res.entries) {
    for (size_t i = 0; i < value.conditions.size(); ++i) {
      auto& condition = value.conditions[i];

      if (condition.comparison != "==" && condition.comparison != "!=" &&
          condition.comparison != ">" && condition.comparison != "<" &&
          condition.comparison != ">=" && condition.comparison != "<=") {
        throw std::runtime_error(fmt::format(
          "Invalid comparison operator: {} (key={}, condition index={})",
          condition.comparison, key, i));
      }

      if (condition.parameter1.empty()) {
        throw std::runtime_error(fmt::format(
          "Empty parameter1 value (key={}, condition index={})", key, i));
      }

      uint32_t parameter1 = 0;

      if (condition.parameter1.find("0x") == 0 ||
          condition.parameter1.find("0X") == 0) {
        std::stringstream ss;
        ss << std::hex << condition.parameter1.substr(2); // Skip "0x"
        ss >> parameter1;
      } else {
        std::stringstream ss(condition.parameter1);
        ss >> parameter1;
      }

      if (parameter1 == 0) {
        throw std::runtime_error(fmt::format(
          "Invalid parameter1 value: {} (key={}, condition index={})",
          condition.parameter1, key, i));
      }

      if (condition.logicalOperator != "OR" &&
          condition.logicalOperator != "AND") {
        throw std::runtime_error(fmt::format(
          "Invalid logical operator: {} (key={}, condition index={})",
          condition.logicalOperator, key, i));
      }
    }
  }

  return res;
}

bool DamageMultConditionalFormula::EvaluateConditions(
  const std::vector<DamageMultConditionalFormulaSettingsValueCondition>&
    conditions,
  const MpActor& aggressor, const MpActor& target) const
{
  std::vector<uint8_t> results(conditions.size(), 0);

  for (size_t i = 0; i < conditions.size(); ++i) {
    const auto& condition = conditions[i];
    results[i] = EvaluateCondition(condition, aggressor, target);
  }
}

bool DamageMultConditionalFormula::EvaluateCondition(
  const DamageMultConditionalFormulaSettingsValueCondition& condition,
  const MpActor& aggressor, const MpActor& target) const
{
  uint32_t parameter1 = ExtractParameter(condition.parameter1);

  std::function<float(MpActor & actor, uint32_t parameter1)>
    conditionFunction = nullptr;

  if (Utils::stricmp(condition.function.data(), "GetGlobalValue") == 0) {
    conditionFunction = [](MpActor& actor, uint32_t parameter1) -> float {
      return 0.f;
    };
  } else if (Utils::stricmp(condition.function.data(), "HasSpell") == 0) {
    conditionFunction = [](MpActor& actor, uint32_t parameter1) -> float {
      auto spelllist = actor.GetSpellList();
      auto it = std::find(spelllist.begin(), spelllist.end(), parameter1);
      if (it != spelllist.end()) {
        return 1.0f;
      }
      return 0.f;
    };
  } else if (Utils::stricmp(condition.function.data(), "GetIsRace") == 0) {
    conditionFunction = [](MpActor& actor, uint32_t parameter1) -> float {
      if (actor.GetRaceId() == parameter1) {
        return 1.0f;
      }
      return 0.f;
    };
  } else {
    conditionFunction = [&](MpActor&, uint32_t) -> float { return 1.0; };
  }
}
