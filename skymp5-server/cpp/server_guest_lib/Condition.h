#pragma once
#include "libespm/CTDA.h"
#include <string>
#include "condition_functions/ConditionFunction.h"

struct Condition
{
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("function", function)
      .Serialize("runsOn", runsOn)
      .Serialize("comparison", comparison)
      .Serialize("value", value)
      .Serialize("parameter1", parameter1)
      .Serialize("parameter2", parameter2)
      .Serialize("logicalOperator", logicalOperator);
  }

  static Condition FromCtda(const espm::CTDA& ctda);

  std::string function;
  std::string runsOn;
  std::string comparison; // ==, !=, >, <, >=, <=
  float value = 0.f;
  std::string parameter1;      // hex uint32_t ("0xDEADBEEF")
  std::string parameter2;      // hex uint32_t ("0xDEADBEEF")
  std::string logicalOperator; // OR, AND
};
