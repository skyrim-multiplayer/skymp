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
  const JsValue& Get(const std::string& propName) const;

  const nlohmann::json& GetAsJson() const;
  static DynamicFields FromJson(const nlohmann::json& j);

private:
  struct Impl;
  std::unique_ptr<Impl, void (*)(Impl*)> pImpl;
};