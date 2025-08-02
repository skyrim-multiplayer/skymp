#include "DynamicFields.h"

#include <unordered_map>

void DynamicFields::SetValueDump(const std::string& propName,
                                 const std::string& valueDump)
{
  jsonCache.reset();
  propDumps[propName] = valueDump;
}

const std::string& DynamicFields::GetValueDump(
  const std::string& propName) const
{
  static const auto kNull = std::string("null");

  auto it = propDumps.find(propName);
  if (it == propDumps.end()) {
    return kNull;
  }

  return it->second;
}

const nlohmann::json& DynamicFields::GetAsJson() const
{
  if (!jsonCache.has_value()) {

    auto obj = nlohmann::json::object();

    for (auto& [key, valueDump] : propDumps) {
      obj[key] = nlohmann::json::parse(valueDump);
    }

    jsonCache = std::move(obj);
  }

  return *jsonCache;
}

DynamicFields DynamicFields::FromJson(const nlohmann::json& j)
{
  DynamicFields res;

  res.jsonCache = j;

  for (auto it = j.begin(); it != j.end(); ++it) {
    res.propDumps[it.key()] = it.value().dump();
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
