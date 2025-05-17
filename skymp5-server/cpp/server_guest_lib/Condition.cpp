#include "Condition.h"
#include <spdlog/spdlog.h>

namespace {
std::string ConvertOperatorToString(espm::CTDA::Operator op)
{
  switch (op) {
    case espm::CTDA::Operator::EqualTo:
      return "==";
    case espm::CTDA::Operator::NotEqualTo:
      return "!=";
    case espm::CTDA::Operator::GreaterThen:
      return ">";
    case espm::CTDA::Operator::GreaterThenOrEqualTo:
      return ">=";
    case espm::CTDA::Operator::LessThen:
      return "<";
    case espm::CTDA::Operator::LessThenOrEqualTo:
      return "<=";
    default:
      spdlog::error("ConvertOperatorToString - Unknown operator: {}",
                    static_cast<int>(op));
      return "";
  }
}

std::string ToStringHexPrefixedUpperCase(uint32_t value)
{
  constexpr auto kValueFmt = "0x%0X";
  size_t size = std::snprintf(nullptr, 0, kValueFmt, value);

  std::vector<char> buffer;
  buffer.resize(size + 1);

  std::sprintf(buffer.data(), kValueFmt, value);

  return { buffer.begin(), buffer.end() };
}

std::string ConvertLogicalOperatorToString(espm::CTDA::Flags flags)
{
  if (static_cast<uint8_t>(flags) &
      static_cast<uint8_t>(espm::CTDA::Flags::OR)) {
    return "OR";
  } else if (static_cast<uint8_t>(flags) &
             static_cast<uint8_t>(espm::CTDA::Flags::ANDORDEFAULT)) {
    return "AND";
  } else {
    throw std::runtime_error("Unknown logical operator");
  }
}

std::string ConvertRunOnTypeToString(espm::CTDA::RunOnTypeFlags runOnType)
{
  switch (runOnType) {
    case espm::CTDA::RunOnTypeFlags::Subject:
      return "Subject";
    case espm::CTDA::RunOnTypeFlags::Target:
      return "Target";
    case espm::CTDA::RunOnTypeFlags::Reference:
      return "Reference";
    case espm::CTDA::RunOnTypeFlags::CombatTarget:
      return "CombatTarget";
    case espm::CTDA::RunOnTypeFlags::LinkedReference:
      return "LinkedReference";
    case espm::CTDA::RunOnTypeFlags::QuestAlias:
      return "QuestAlias";
    case espm::CTDA::RunOnTypeFlags::PackageData:
      return "PackageData";
    case espm::CTDA::RunOnTypeFlags::EventData:
      return "EventData";
    default:
      throw std::runtime_error("Unknown run on type");
  }
}

std::string ConvertFunctionIndexToString(uint16_t functionIndex)
{
  switch (functionIndex) {
    case 47:
      return "GetItemCount";
    case 69:
      return "GetIsRace";
    case 448:
      return "HasPerk";
    case 576:
      return "GetEventData";
    case 264:
      return "HasSpell";
    case 682:
      return "WornHasKeyword";
    case 182:
      return "GetEquipped";
    default:
      spdlog::warn("ConvertFunctionIndexToString - Unknown function index: {}",
                   functionIndex);
      return "";
  }
}
}

Condition Condition::FromCtda(const espm::CTDA& ctda)
{
  Condition condition;

  condition.comparison = ConvertOperatorToString(ctda.GetOperator());
  condition.value = ctda.comparisonValue;

  condition.parameter1 =
    ToStringHexPrefixedUpperCase(ctda.GetDefaultData().firstParameter);
  condition.parameter2 =
    ToStringHexPrefixedUpperCase(ctda.GetDefaultData().secondParameter);

  condition.logicalOperator = ConvertLogicalOperatorToString(ctda.GetFlags());
  condition.runsOn = ConvertRunOnTypeToString(ctda.runOnType);

  condition.function = ConvertFunctionIndexToString(ctda.functionIndex);

  return condition;
}
