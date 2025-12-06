#include "ConditionsEvaluator.h"
#include "MpActor.h"
#include "archives/JsonInputArchive.h"
#include "condition_functions/ConditionFunctionMap.h"
#include "papyrus-vm/Utils.h"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>

ConditionsEvaluatorSettings ConditionsEvaluatorSettings::FromJson(
  const nlohmann::json& j)
{
  JsonInputArchive ar(j);
  ConditionsEvaluatorSettings valueParsed;
  valueParsed.Serialize(ar);

  return valueParsed;
}

void ConditionsEvaluator::EvaluateConditions(
  const ConditionFunctionMap& conditionFunctionMap,
  const ConditionsEvaluatorSettings& settings,
  ConditionsEvaluatorCaller caller, const std::vector<Condition>& conditions,
  const MpActor& aggressor, const MpActor& target,
  const std::function<void(bool, std::vector<std::string>&)>& callback,
  const ConditionEvaluatorContext& context)
{
  bool enableLogging = false;

  for (auto& callerToLog : settings.callersToLog) {
    if (callerToLog == "DamageMultConditionalFormula" &&
        caller == ConditionsEvaluatorCaller::kDamageMultConditionalFormula) {
      enableLogging = true;
      break;
    }
    if (callerToLog == "Craft" &&
        caller == ConditionsEvaluatorCaller::kCraft) {
      enableLogging = true;
      break;
    }
  }

  std::vector<int> conditionResolutions;
  std::vector<float> conditionFunctionResults;

  const bool evalRes = ConditionsEvaluator::EvaluateConditionsImpl(
    conditionFunctionMap, conditions,
    enableLogging ? &conditionResolutions : nullptr,
    enableLogging ? &conditionFunctionResults : nullptr, aggressor, target,
    context);

  std::vector<std::string> strings;

  if (enableLogging) {
    strings = ConditionsEvaluator::LogEvaluateConditionsResolution(
      conditions, conditionResolutions, conditionFunctionResults, evalRes);
  }

  callback(evalRes, strings);

  if (!strings.empty()) {
    spdlog::info("{}", fmt::join(strings.begin(), strings.end(), "\n"));
  }
}

bool ConditionsEvaluator::EvaluateConditionsImpl(
  const ConditionFunctionMap& conditionFunctionMap,
  const std::vector<Condition>& conditions,
  std::vector<int>* outConditionResolutions,
  std::vector<float>* outConditionFunctionResults, const MpActor& aggressor,
  const MpActor& target, const ConditionEvaluatorContext& context)
{
  auto conditionResolutions = outConditionResolutions
    ? std::vector<int>(conditions.size(), -1)
    : std::vector<int>();

  auto conditionFunctionResults = outConditionFunctionResults
    ? std::vector<float>(conditions.size(), 0)
    : std::vector<float>();

  bool good = false;

  for (size_t i = 0; i < conditions.size(); ++i) {
    const auto& condition = conditions[i];

    if (!good) {
      std::pair<bool, float> pair = EvaluateCondition(
        conditionFunctionMap, condition, aggressor, target, context);

      uint8_t evaluateConditionResult = pair.first ? 1 : 0;

      if (conditionResolutions.size() > i) {
        conditionResolutions[i] = evaluateConditionResult;
      }
      if (conditionFunctionResults.size() > i) {
        conditionFunctionResults[i] = pair.second;
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

        if (outConditionFunctionResults) {
          std::swap(conditionFunctionResults, *outConditionFunctionResults);
        }

        return false;
      }
      good = false;
    }
  }

  if (outConditionResolutions) {
    std::swap(conditionResolutions, *outConditionResolutions);
  }

  if (outConditionFunctionResults) {
    std::swap(conditionFunctionResults, *outConditionFunctionResults);
  }

  return true;
}

std::vector<std::string> ConditionsEvaluator::LogEvaluateConditionsResolution(
  const std::vector<Condition>& conditions,
  const std::vector<int>& conditionResolutions,
  const std::vector<float>& conditionFunctionResults, bool finalResult)
{
  if (conditions.size() != conditionResolutions.size() ||
      conditions.size() != conditionFunctionResults.size()) {
    spdlog::error(
      "ConditionsEvaluator::LogEvaluateConditionsResolution - Mismatched conditions and results sizes: {} != {} != {}",
      conditions.size(), conditionResolutions.size(), conditionFunctionResults.size());
    return std::vector<std::string>();
  }

  auto getAlphabetChar = [](size_t index) -> std::string {
    // 1. Determine the letter (ALWAYS A-Z)
    char letter = 'A' + static_cast<char>(index % 26);
    
    std::string res(1, letter);

    // 2. Determine the cycle number (Empty, then 1, 2, 3...)
    // 0-25 (A-Z)   -> 0 (Do nothing)
    // 26-51 (A1-Z1) -> 1 (Add "1")
    // 52-77 (A2-Z2) -> 2 (Add "2")
    if (index >= 26) {
      res += std::to_string(index / 26);
    }

    return res;
  };

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
  size_t alphabetCharCounter = 0;

  for (size_t i = 0; i < conditions.size(); ++i) {
    if (groupStarts.size() > currentGroupStart &&
        groupStarts[currentGroupStart] == i) {
      ++currentGroupStart;
      if (s.empty()) {
        s += "(";
      } else {
        s.pop_back(); // remove last space
        s += ") & (";
      }
    }

    s += getAlphabetChar(alphabetCharCounter);
    ++alphabetCharCounter;

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

  alphabetCharCounter = 0;

  for (size_t i = 0; i < conditions.size(); ++i) {
    s.clear();
    s += getAlphabetChar(alphabetCharCounter);
    s += ": ";
    s += conditions[i].function;
    s += ' ';
    s += conditions[i].parameter1;
    s += ' ';
    s += std::to_string(conditionFunctionResults[i]);
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

    ++alphabetCharCounter;
  }

  return res;
}

std::pair<bool, float> ConditionsEvaluator::EvaluateCondition(
  const ConditionFunctionMap& conditionFunctionMap, const Condition& condition,
  const MpActor& aggressor, const MpActor& target,
  const ConditionEvaluatorContext& context)
{
  uint32_t parameter1 = ExtractParameter(condition.parameter1);
  uint32_t parameter2 = ExtractParameter(condition.parameter2);

  auto conditionFunction =
    conditionFunctionMap.GetConditionFunction(condition.function.data());

  MpActor* runsOn = nullptr;

  if (condition.runsOn == "Subject") {
    // TODO: get rid of const_cast
    runsOn = const_cast<MpActor*>(&aggressor);
  } else if (condition.runsOn == "Target") {
    // TODO: get rid of const_cast
    runsOn = const_cast<MpActor*>(&target);
  } else if (condition.runsOn == "Reference") {
    // TODO: get rid of const_cast
    // TODO: fix implementation. must read formId somewhere (uesp is unclear
    // about that). and use that formId. Why this hotfix works, because we
    // usually use 0x14 (PlayerRef) as a reference.
    runsOn = const_cast<MpActor*>(&aggressor);
  } else {
    // TODO: other options
    // TODO: condier using polymorphism instead of if/else logic
    // TODO: consider proper error handling instead of magic -108.f
    return { false, -108.f };
  }

  if (!conditionFunction) {
    spdlog::warn("ConditionsEvaluator::EvaluateCondition - Condition function "
                 "'{}' doesn't exist. Evaluating condition to True",
                 condition.function);
    return { true, -108.f };
  }

  const float conditionFunctionResult =
    conditionFunction->Execute(*runsOn, parameter1, parameter2, context);

  float valueToCompareWith = condition.value;
  const std::string& comparison = condition.comparison;

  bool comparisonResult =
    CompareFloats(conditionFunctionResult, valueToCompareWith, comparison);

  return { comparisonResult, conditionFunctionResult };
}

bool ConditionsEvaluator::CompareFloats(float a, float b,
                                        const std::string& op)
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

uint32_t ConditionsEvaluator::ExtractParameter(const std::string& parameter)
{
  const char* str = parameter.c_str();
  char* end;
  int base = 10;

  if (parameter.length() > 2 && str[0] == '0') {
    if (str[1] == 'x' || str[1] == 'X') {
      base = 16;
    } else if (str[1] == 'b' || str[1] == 'B') {
      base = 2;
      str += 2; // strtoul doesn't skip 0b automatically
    }
  }

  return static_cast<uint32_t>(std::strtoul(str, &end, base));
}
