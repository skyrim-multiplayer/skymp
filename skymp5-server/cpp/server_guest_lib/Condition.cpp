#include "Condition.h"

bool CalculateOperationResult(int firstArgument, int secondArgument,
                              espm::CTDA::Operator conditionOperator)
{
  switch (conditionOperator) {
    case espm::CTDA::Operator::EqualTo:
      return firstArgument == secondArgument;
    case espm::CTDA::Operator::NotEqualTo:
      return firstArgument != secondArgument;
    case espm::CTDA::Operator::GreaterThen:
      return firstArgument > secondArgument;
    case espm::CTDA::Operator::GreaterThenOrEqualTo:
      return firstArgument >= secondArgument;
    case espm::CTDA::Operator::LessThen:
      return firstArgument < secondArgument;
    case espm::CTDA::Operator::LessThenOrEqualTo:
      return firstArgument <= secondArgument;
    default:
      return false;
  }
}

Condition::Condition(espm::CTDA::Operator conditionOperator,
                     espm::CTDA::Flags flags)
  : conditionOperator(conditionOperator)
  , flags(flags)
{
}

espm::CTDA::Flags Condition::GetFlags() const
{
  return flags;
}

ItemCountCondition::ItemCountCondition(int itemId, int comparisonValue,
                                       espm::CTDA::Operator conditionOperator,
                                       espm::CTDA::Flags flags)
  : Condition(conditionOperator, flags)
  , itemId(itemId)
  , comparisonValue(comparisonValue)
{
}

bool ItemCountCondition::Evaluate(MpActor* actor) const
{
  int itemCount = actor->GetInventory().GetItemCount(itemId);
  bool result =
    CalculateOperationResult(itemCount, comparisonValue, conditionOperator);
  spdlog::info("ItemCountCondition: itemId = {}, itemCount = {}, "
               "comparisonValue = {}, operator = {}, result = {}",
               itemId, itemCount, comparisonValue,
               static_cast<int>(conditionOperator), result);
  return result;
}

std::string ItemCountCondition::GetDescription() const
{
  return "ItemCountCondition: itemId = " + std::to_string(itemId) +
    ", comparisonValue = " + std::to_string(comparisonValue) +
    ", operator = " + std::to_string(static_cast<int>(conditionOperator));
}

RaceCondition::RaceCondition(int raceId, int comparisonValue,
                             espm::CTDA::Operator conditionOperator,
                             espm::CTDA::Flags flags)
  : Condition(conditionOperator, flags)
  , raceId(raceId)
  , comparisonValue(comparisonValue)
{
}

bool RaceCondition::Evaluate(MpActor* actor) const
{
  int raceEquals = actor->GetRaceId() == raceId ? 1 : 0;
  bool result =
    CalculateOperationResult(raceEquals, comparisonValue, conditionOperator);
  spdlog::info("RaceCondition: raceId = {}, raceEquals = {}, "
               "comparisonValue = {}, operator = {}, result = {}",
               raceId, raceEquals, comparisonValue,
               static_cast<int>(conditionOperator), result);
  return result;
}

std::string RaceCondition::GetDescription() const
{
  return "RaceCondition: raceId = " + std::to_string(raceId) +
    ", comparisonValue = " + std::to_string(comparisonValue) +
    ", operator = " + std::to_string(static_cast<int>(conditionOperator));
}
