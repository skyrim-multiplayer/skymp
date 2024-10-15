#include "HookPattern.h"
#include <algorithm>
#include <stdexcept>
#include <string>

HookPattern HookPattern::Parse(const std::string& str)
{
  auto count = std::count(str.begin(), str.end(), '*');
  if (count == 0) {
    return { HookPatternType::Exact, str };
  }
  if (count > 1) {
    throw std::runtime_error(
      "Patterns can contain only one '*' at the beginning/end of string");
  }

  auto pos = str.find('*');
  if (pos == 0) {
    return { HookPatternType::EndsWith,
             std::string(str.begin() + 1, str.end()) };
  }
  if (pos == str.size() - 1) {
    return { HookPatternType::StartsWith,
             std::string(str.begin(), str.end() - 1) };
  }
  throw std::runtime_error(
    "In patterns '*' must be at the beginning/end of string");
}
