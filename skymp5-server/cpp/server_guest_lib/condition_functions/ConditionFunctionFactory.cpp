#include "ConditionFunctionFactory.h"

#include "GetEquipped.h"
#include "GetIsRace.h"
#include "GetItemCount.h"
#include "HasSpell.h"
#include "WornHasKeyword.h"

ConditionFunctionMap ConditionFunctionFactory::CreateConditionFunctions()
{
  ConditionFunctionMap res;

  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::GetEquipped>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::GetIsRace>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::GetItemCount>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::HasSpell>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::WornHasKeyword>());

  return res;
}
