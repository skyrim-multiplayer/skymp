#include "DamageMultConditionalFormula.h"

#include "MpActor.h"
#include "archives/JsonInputArchive.h"
#include "papyrus-vm/Utils.h"
#include <fmt/format.h>
#include <fmt/ranges.h>
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
      // TODO
      bool enableLogging = false;

      std::vector<int> conditionResolutions;

      bool evalRes = EvaluateConditions(
        value.conditions, enableLogging ? &conditionResolutions : nullptr,
        aggressor, target);
      if (evalRes) {
        baseDamage *= *value.physicalDamageMultiplier;
      }

      if (enableLogging) {
        std::vector<std::string> strings = LogEvaluateConditionsResolution(
          value.conditions, conditionResolutions, evalRes);

        if (evalRes) {
          strings.insert(strings.begin(),
                         fmt::format("Damage multiplier: {} (key={})",
                                     *value.physicalDamageMultiplier, key));
        } else {
          strings.insert(
            strings.begin(),
            fmt::format("Damage multiplier: {} (key={}, evalRes=false)", 1.f,
                        key));
        }

        spdlog::info("{}", fmt::join(strings.begin(), strings.end(), "\n"));
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
      // TODO
      bool enableLogging = false;

      std::vector<int> conditionResolutions;

      bool evalRes = EvaluateConditions(
        value.conditions, enableLogging ? &conditionResolutions : nullptr,
        aggressor, target);
      if (evalRes) {
        baseDamage *= *value.magicDamageMultiplier;
      }

      if (enableLogging) {
        std::vector<std::string> strings = LogEvaluateConditionsResolution(
          value.conditions, conditionResolutions, evalRes);

        if (evalRes) {
          strings.insert(strings.begin(),
                         fmt::format("Damage multiplier: {} (key={})",
                                     *value.magicDamageMultiplier, key));
        } else {
          strings.insert(
            strings.begin(),
            fmt::format("Damage multiplier: {} (key={}, evalRes=false)", 1.f,
                        key));
        }

        spdlog::info("{}", fmt::join(strings.begin(), strings.end(), "\n"));
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

    JsonInputArchive ar(value);
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
  std::vector<int>* outConditionResolutions, const MpActor& aggressor,
  const MpActor& target) const
{
  auto conditionResolutions = outConditionResolutions
    ? std::vector<int>(conditions.size(), -1)
    : std::vector<int>();

  bool good = false;

  for (size_t i = 0; i < conditions.size(); ++i) {
    const auto& condition = conditions[i];

    if (!good) {
      uint8_t evaluateConditionResult =
        EvaluateCondition(condition, aggressor, target) ? 1 : 0;

      if (conditionResolutions.size() > i) {
        conditionResolutions[i] = evaluateConditionResult;
      }

      if (evaluateConditionResult > 0) {
        good = true;
      }
    }

    if (condition.logicalOperator == "AND" || i == conditions.size() - 1) {
      if (!good) {
        if (outConditionResolutions) {
          std::swap(conditionResolutions, *outConditionResolutions);
        }
        return false;
      }
      good = false;
    }
  }

  if (outConditionResolutions) {
    std::swap(conditionResolutions, *outConditionResolutions);
  }

  return true;
}

std::vector<std::string>
DamageMultConditionalFormula::LogEvaluateConditionsResolution(
  const std::vector<DamageMultConditionalFormulaSettingsValueCondition>&
    conditions,
  const std::vector<int>& conditionResolutions, bool finalResult) const
{
  std::vector<std::string> res;

  std::string s;

  std::vector<size_t> groupStarts = { 0 };

  for (size_t i = 0; i < conditions.size(); ++i) {
    const auto& condition = conditions[i];

    if (i != conditions.size() - 1 && condition.logicalOperator == "AND") {
      groupStarts.push_back(i + 1);
    }
  }

  size_t currentGroupStart = 0;
  char nextAlphabetChar = 'A';

  for (size_t i = 0; i < conditions.size(); ++i) {
    if (groupStarts.size() > currentGroupStart &&
        groupStarts[currentGroupStart] == i) {
      currentGroupStart = i + 1;
      if (s.empty()) {
        s += "(";
      } else {
        s.pop_back();
        s += ") & (";
      }
    }

    s += nextAlphabetChar++;

    if (i != conditions.size() - 1) {
      s += conditions[i].logicalOperator == "AND" ? " " : " | ";
    }
  }

  if (!s.empty()) {
    s += ')';
  }

  s += " = " + std::to_string(finalResult);

  res.push_back(s);

  res.push_back("Condition resolutions:");

  nextAlphabetChar = 'A';

  for (size_t i = 0; i < conditions.size(); ++i) {
    s.clear();
    s += nextAlphabetChar++;
    s += ": ";
    s += conditions[i].function;
    s += ' ';
    s += conditions[i].parameter1;
    s += ' ';
    s += conditions[i].comparison;
    s += ' ';
    s += std::to_string(conditions[i].value);
    s += " -> ";
    if (conditionResolutions[i] == 1) {
      s += "true";
    } else if (conditionResolutions[i] == 0) {
      s += "false";
    } else {
      s += "unknown";
    }
    res.push_back(s);
  }

  return res;
}

bool DamageMultConditionalFormula::EvaluateCondition(
  const DamageMultConditionalFormulaSettingsValueCondition& condition,
  const MpActor& aggressor, const MpActor& target) const
{
  uint32_t parameter1 = ExtractParameter(condition.parameter1);

  std::function<float(const MpActor& actor, uint32_t parameter1)>
    conditionFunction = nullptr;

  // if (Utils::stricmp(condition.function.data(), "GetGlobalValue") == 0) {
  //   conditionFunction = [](MpActor& actor, uint32_t parameter1) -> float {
  //     return 0.f;
  //   };
  // }

  if (Utils::stricmp(condition.function.data(), "HasSpell") == 0) {
    conditionFunction = [](const MpActor& actor,
                           uint32_t parameter1) -> float {
      auto spelllist = actor.GetSpellList();
      auto it = std::find(spelllist.begin(), spelllist.end(), parameter1);
      if (it != spelllist.end()) {
        return 1.0f;
      }
      return 0.f;
    };
  } else if (Utils::stricmp(condition.function.data(), "GetIsRace") == 0) {
    conditionFunction = [](const MpActor& actor,
                           uint32_t parameter1) -> float {
      if (actor.GetRaceId() == parameter1) {
        return 1.0f;
      }
      return 0.f;
    };
  } else {
    conditionFunction = [&](const MpActor&, uint32_t) -> float { return 1.0; };
  }

  const MpActor* runsOn = nullptr;

  if (condition.runsOn == "Subject") {
    runsOn = &aggressor;
  } else if (condition.runsOn == "Target") {
    runsOn = &target;
  } else {
    // TODO: other options
    return false;
  }

  float conditionFunctionResult = conditionFunction(*runsOn, parameter1);

  float valueToCompareWith = condition.value;
  const std::string& comparison = condition.comparison;

  bool comparisonResult =
    CompareFloats(conditionFunctionResult, valueToCompareWith, comparison);

  return comparisonResult;
}

bool DamageMultConditionalFormula::CompareFloats(float a, float b,
                                                 const std::string& op) const
{
  // No idea how the real engine does it. I'm adding epsion for safety.

  constexpr float kEpsilon = std::numeric_limits<float>::epsilon();
  if (op == "==") {
    return std::fabs(a - b) < kEpsilon;
  }
  if (op == "!=") {
    return std::fabs(a - b) >= kEpsilon;
  }
  if (op == "<") {
    return a < b - kEpsilon;
  }
  if (op == ">") {
    return a > b + kEpsilon;
  }
  if (op == "<=") {
    return a < b + kEpsilon || std::fabs(a - b) < kEpsilon;
  }
  if (op == ">=") {
    return a > b - kEpsilon || std::fabs(a - b) < kEpsilon;
  }
  return false;
}
