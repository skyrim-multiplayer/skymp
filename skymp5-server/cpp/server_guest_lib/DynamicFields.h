#pragma once
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

class DynamicFields
{
public:
  void Set(const std::string& propName, const nlohmann::json& value);
  const nlohmann::json& Get(const std::string& propName) const;

  const nlohmann::json& GetAsJson() const;
  static DynamicFields FromJson(const nlohmann::json& j);

  template <class F>
  void ForEach(const F& f) const
  {
    for (auto [propName, value] : props) {
      f(propName, value);
    }
  }

  friend bool operator<(const DynamicFields& r, const DynamicFields& l);
  friend bool operator==(const DynamicFields& r, const DynamicFields& l);
  friend bool operator!=(const DynamicFields& r, const DynamicFields& l);

private:
  std::unordered_map<std::string, nlohmann::json> props;
  mutable std::optional<nlohmann::json> jsonCache;
};
