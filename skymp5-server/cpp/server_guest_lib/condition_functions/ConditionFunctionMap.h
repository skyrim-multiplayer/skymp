#pragma once
#include "ConditionFunction.h"
#include "papyrus-vm/Utils.h" // CIMap
#include <memory>
#include <vector>

class ConditionFunctionMap
{
public:
  void RegisterConditionFunction(std::shared_ptr<ConditionFunction> function);

  const std::shared_ptr<ConditionFunction>& GetConditionFunction(
    const char* name) const;

  const std::shared_ptr<ConditionFunction>& GetConditionFunction(
    uint16_t functionIndex) const;

  const std::vector<std::shared_ptr<ConditionFunction>>&
  GetConditionFunctions() const;

  template <class F>
  void ForEachConditionFunction(F&& func)
  {
    for (const auto& conditionFunction : conditionFunctions) {
      func(conditionFunction);
    }
  }

private:
  CIMap<std::shared_ptr<ConditionFunction>> conditionFunctionByName;
  std::vector<std::shared_ptr<ConditionFunction>> conditionFunctionByIndex;
  std::vector<std::shared_ptr<ConditionFunction>> conditionFunctions;
};
