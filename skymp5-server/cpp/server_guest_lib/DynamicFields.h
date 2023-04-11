#pragma once
#include <cstdint>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>

class DynamicFields
{
public:
  DynamicFields();
  DynamicFields(const DynamicFields& rhs);
  DynamicFields& operator=(const DynamicFields& rhs);

  void Set(const std::string &propName, const nlohmann::json& value);
  nlohmann::json Get(const std::string &propName) const;

  const nlohmann::json& GetAsJson() const;
  static DynamicFields FromJson(const nlohmann::json& j);

  friend bool operator<(const DynamicFields& r, const DynamicFields& l);
  friend bool operator==(const DynamicFields& r, const DynamicFields& l);
  friend bool operator!=(const DynamicFields& r, const DynamicFields& l);

private:
  struct Impl;
  std::unique_ptr<Impl, void (*)(Impl*)> pImpl;
};
