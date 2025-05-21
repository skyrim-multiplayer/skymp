#include "ConditionFunctionMap.h"

#include <fmt/format.h>
#include <stdexcept>

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

  bool isDuplicateName =
    conditionFunctionByName.find(name) != conditionFunctionByName.end();
  if (isDuplicateName) {
    throw std::runtime_error(
      fmt::format("Condition function with name '{}' already exists", name));
  }

  bool isDuplicateIndex =
    function->GetFunctionIndex() < conditionFunctionByIndex.size() &&
    conditionFunctionByIndex[function->GetFunctionIndex()] != nullptr;
  if (isDuplicateIndex) {
    throw std::runtime_error(
      fmt::format("Condition function with index '{}' already exists",
                  function->GetFunctionIndex()));
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
