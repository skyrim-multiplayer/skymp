#include "DynamicFields.h"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#include <optional>

struct DynamicFields::Impl
{
  std::unordered_map<std::string, nlohmann::json> props;
  std::optional<nlohmann::json> jsonCache;
};

DynamicFields::DynamicFields()
  : pImpl(new Impl, [](Impl* ptr) { delete ptr; })
{
}

DynamicFields::DynamicFields(const DynamicFields& rhs)
  : pImpl(nullptr, [](Impl* ptr) { delete ptr; })
{
  *this = rhs;
}

DynamicFields& DynamicFields::operator=(const DynamicFields& rhs)
{
  pImpl.reset(new Impl(*rhs.pImpl));
  return *this;
}

void DynamicFields::Set(const std::string& propName, const nlohmann::json& value)
{
  pImpl->jsonCache.reset();
  pImpl->props[propName] = value;
}

nlohmann::json DynamicFields::Get(const std::string &propName) const {
  auto it = pImpl->props.find(propName);
  if (it == pImpl->props.end()) {
    return nlohmann::json();
  }
  return it->second;
}

const nlohmann::json& DynamicFields::GetAsJson() const
{
  if (!pImpl->jsonCache.has_value()) {

    auto obj = nlohmann::json::object();

    for (auto& [key, v] : pImpl->props) {
      obj[key] = v;
    }

    pImpl->jsonCache = std::move(obj);
  }
  return *pImpl->jsonCache;
}

DynamicFields DynamicFields::FromJson(const nlohmann::json& j)
{
  DynamicFields res;
  for (auto it = j.begin(); it != j.end(); ++it) {
    res.pImpl->props[it.key()] = it.value();
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
