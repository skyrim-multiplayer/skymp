#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

class DynamicFields
{
public:
  void SetValueDump(const std::string& propName, const std::string& valueDump);
  const std::string& GetValueDump(const std::string& propName) const;

  const nlohmann::json& GetAsJson() const;
  static DynamicFields FromJson(const nlohmann::json& j);

  template <class F>
  void ForEachValueDump(const F& f) const
  {
    for (auto [propName, valueDump] : propDumps) {
      f(propName, valueDump);
    }
  }

  friend bool operator<(const DynamicFields& r, const DynamicFields& l);
  friend bool operator==(const DynamicFields& r, const DynamicFields& l);
  friend bool operator!=(const DynamicFields& r, const DynamicFields& l);

private:
  std::unordered_map<std::string, std::string> propDumps;
  mutable std::optional<nlohmann::json> jsonCache;
};
