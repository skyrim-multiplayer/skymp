#pragma once
#include <cstdint>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>

class JsValue;

class DynamicFields
{
public:
  DynamicFields();
  DynamicFields(const DynamicFields& rhs);
  DynamicFields& operator=(const DynamicFields& rhs);

  void Set(const std::string& propName, const JsValue& value);
  JsValue Get(const std::string& propName) const;

  const nlohmann::json& GetAsJson() const;
  static DynamicFields FromJson(const nlohmann::json& j);

  friend bool operator<(const DynamicFields& r, const DynamicFields& l);
  friend bool operator==(const DynamicFields& r, const DynamicFields& l);
  friend bool operator!=(const DynamicFields& r, const DynamicFields& l);

private:
  struct Impl;
  std::unique_ptr<Impl, void (*)(Impl*)> pImpl;
};
