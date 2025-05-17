#pragma once
#include "Condition.h"
#include <functional>
#include <string>
#include <vector>

class MpActor;

class ConditionsEvaluator
{
public:
  static bool EvaluateConditions(const std::vector<Condition>& conditions,
                                 std::vector<int>* outConditionResolutions,
                                 const MpActor& aggressor,
                                 const MpActor& target);

  static std::vector<std::string> LogEvaluateConditionsResolution(
    const std::vector<Condition>& conditions,
    const std::vector<int>& conditionResolutions, bool finalResult);

  static bool EvaluateCondition(const Condition& condition,
                                const MpActor& aggressor,
                                const MpActor& target);

  static bool CompareFloats(float a, float b, const std::string& op);

  static uint32_t ExtractParameter1(const std::string& parameter1);
};
