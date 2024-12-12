#include "DynamicFields.h"

#include <unordered_map>

void DynamicFields::Set(const std::string& propName,
                        nlohmann::json value)
{
  jsonCache.reset();
  props[propName] = std::move(value);
}

const nlohmann::json& DynamicFields::Get(const std::string& propName) const
{
  static const auto kNull = nlohmann::json();

  auto it = props.find(propName);
  if (it == props.end()) {
    return kNull;
  }
  return it->second;
}

const nlohmann::json& DynamicFields::GetAsJson() const
{
  if (!jsonCache.has_value()) {

    auto obj = nlohmann::json::object();

    for (auto& [key, v] : props) {
      obj[key] = v;
    }

    jsonCache = std::move(obj);
  }
  return *jsonCache;
}

DynamicFields DynamicFields::FromJson(const nlohmann::json& j)
{
  DynamicFields res;
  for (auto it = j.begin(); it != j.end(); ++it) {
    res.props[it.key()] = it.value();
  }
  return res;
}

bool operator<(const DynamicFields& r, const DynamicFields& l)
{
  return r.GetAsJson() < l.GetAsJson();
}

bool operator==(const DynamicFields& r, const DynamicFields& l)
{
  return r.GetAsJson() == l.GetAsJson();
}

bool operator!=(const DynamicFields& r, const DynamicFields& l)
{
  return !(r == l);
}
