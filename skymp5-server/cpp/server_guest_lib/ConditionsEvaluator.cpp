#include "ConditionsEvaluator.h"

bool ConditionsEvaluator::EvaluateConditions(
  const std::vector<Condition>&
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
ConditionsEvaluator::LogEvaluateConditionsResolution(
  const std::vector<Condition>&
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

bool ConditionsEvaluator::EvaluateCondition(
  const Condition& condition,
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
  } else if (Utils::stricmp(condition.function.data(), "GetItemCount") == 0) {
    conditionFunction = [](const MpActor& actor,
                           uint32_t parameter1) -> float {
      return static_cast<float>(actor.GetInventory().GetItemCount(parameter1));
    };
  } else if (Utils::stricmp(condition.function.data(), "WornHasKeyword") == 0) {
    conditionFunction = [](const MpActor& actor,
                           uint32_t parameter1) -> float {
      auto& br = actor.GetParent()->GetEspm().GetBrowser();
      PapyrusActor papyrusActor;
      auto aKeyword = VarValue(std::make_shared<EspmGameObject>(br.LookupById(parameter1)));

      VarValue res = papyrusActor.WornHasKeyword(actor.ToVarValue(), { aKeyword });
      bool resBool = static_cast<bool>(res);
      if (resBool) {
        return 1.0f;
      }
      return 0.f;
    };
  } else if (Utils::stricmp(condition.function.data(), "GetEquipped") == 0) {
    conditionFunction = [](const MpActor& actor,
                           uint32_t parameter1) -> float {
    if (actor.GetEquipment().inv.GetItemCount(parameter1) > 0) {
      return 1.f;
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

bool ConditionsEvaluator::CompareFloats(float a, float b,
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
