#include "ConditionFunctionMap.h"

void ConditionFunctionMap::RegisterConditionFunction(
  std::shared_ptr<ConditionFunction> function)
{
  if (!function) {
    return;
  }

  const char* name = function->GetName();
  if (name == nullptr) {
    return;
  }

  conditionFunctionByName[name] = function;

  uint16_t functionIndex = function->GetFunctionIndex();
  if (functionIndex >= conditionFunctionByIndex.size()) {
    conditionFunctionByIndex.resize(functionIndex + 1);
  }
  conditionFunctionByIndex[functionIndex] = function;

  conditionFunctions.push_back(function);
}

const std::shared_ptr<ConditionFunction>&
ConditionFunctionMap::GetConditionFunction(const char* name) const
{
  auto it = conditionFunctionByName.find(name);
  if (it != conditionFunctionByName.end()) {
    return it->second;
  }
  static const std::shared_ptr<ConditionFunction> kNullFunction;
  return kNullFunction;
}

const std::shared_ptr<ConditionFunction>&
ConditionFunctionMap::GetConditionFunction(uint16_t functionIndex) const
{
  if (functionIndex < conditionFunctionByIndex.size()) {
    return conditionFunctionByIndex[functionIndex];
  }
  static const std::shared_ptr<ConditionFunction> kNullFunction;
  return kNullFunction;
}

const std::vector<std::shared_ptr<ConditionFunction>>&
ConditionFunctionMap::GetConditionFunctions() const
{
  return conditionFunctions;
}
