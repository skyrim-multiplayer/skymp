#pragma once

enum class HookPatternType
{
  Exact,
  StartsWith,
  EndsWith
};

class HookPattern
{
public:
  static HookPattern Parse(const std::string& str);

  HookPatternType type;
  std::string str;
};
