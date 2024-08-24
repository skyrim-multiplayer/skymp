#include "DynamicFields.h"
#include <unordered_map>
#include <vector>

void DynamicFields::Set(const std::string& propName,
                        const DynamicFieldsEntryValue& value)
{
  jsonCache.reset();
  props[propName] = value;
}

const DynamicFieldsEntryValue& DynamicFields::Get(
  const std::string& propName) const
{
  static const DynamicFieldsEntryValue kNullEntry = DynamicFieldsValueObject();

  auto it = props.find(propName);
  if (it == props.end()) {
    return kNullEntry;
  }
  return it->second;
}

const nlohmann::json& DynamicFields::GetAsJson() const
{
  if (!jsonCache.has_value()) {

    auto obj = nlohmann::json::object();

    for (auto& [key, v] : props) {
      obj[key] = DynamicFields::ToJson(v);
    }

    jsonCache = std::move(obj);
  }
  return *jsonCache;
}

DynamicFields DynamicFields::FromJson(const nlohmann::json& j)
{
  DynamicFields res;
  // for (auto it = j.begin(); it != j.end(); ++it) {
  //   res.props[it.key()] = it.value();
  // }
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

nlohmann::json DynamicFields::ToJson(const DynamicFieldsEntryValue& value)
{
}