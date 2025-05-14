#pragma once

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
      .Serialize("logicalOperator", logicalOperator);
  }

  std::string function;
  std::string runsOn;
  std::string comparison; // ==, !=, >, <, >=, <=
  float value = 0.f;
  std::string parameter1;      // hex uint32_t
  std::string logicalOperator; // OR, AND
};