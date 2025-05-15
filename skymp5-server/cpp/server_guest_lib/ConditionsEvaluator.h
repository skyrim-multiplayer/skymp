#pragma once

class ConditionsEvaluator
{
public: 

static bool EvaluateConditions(
    const std::vector<DamageMultConditionalFormulaSettingsValueCondition>&
      conditions,
    std::vector<int>* outConditionResolutions, const MpActor& aggressor,
    const MpActor& target);

  static std::vector<std::string> LogEvaluateConditionsResolution(
    const std::vector<DamageMultConditionalFormulaSettingsValueCondition>&
      conditions,
    const std::vector<int>& conditionResolutions, bool finalResult);

  static bool EvaluateCondition(
    const DamageMultConditionalFormulaSettingsValueCondition& condition,
    const MpActor& aggressor, const MpActor& target) const;

  static bool CompareFloats(float a, float b, const std::string& op);

};