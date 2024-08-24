#pragma once
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>

enum class DynamicFieldsEntryCacheIndex
{
  kObject,
  kDouble,
  kBool,
  kString
};

struct DynamicFieldsValueObject
{
  void* napiValue = nullptr;
  std::string jsonDump;
};

using DynamicFieldsEntryValue =
  std::variant<DynamicFieldsValueObject, double, bool, std::string> value;

class DynamicFields
{
public:
  void Set(const std::string& propName, const DynamicFieldsEntryValue& value);
  const DynamicFieldsEntryValue& Get(const std::string& propName) const;

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
  static nlohmann::json ToJson(const DynamicFieldsEntryValue& value);

  std::unordered_map<std::string, DynamicFieldsEntryValue> props;
  mutable std::optional<nlohmann::json> jsonCache;
};
