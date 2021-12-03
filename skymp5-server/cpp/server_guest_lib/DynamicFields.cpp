#include "DynamicFields.h"
#include <JsEngine.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

namespace {
nlohmann::json JsValueToJson(const JsValue& v)
{
  JsValue stringify =
    JsValue::GlobalObject().GetProperty("JSON").GetProperty("stringify");
  std::vector<JsValue> args = { JsValue::Undefined(), JsValue::Undefined() };
  args[1] = v;
  auto dump = static_cast<std::string>(stringify.Call(args));
  return nlohmann::json::parse(dump);
}

JsValue JsonToJsValue(const nlohmann::json& j)
{
  JsValue parse =
    JsValue::GlobalObject().GetProperty("JSON").GetProperty("parse");
  std::vector<JsValue> args = { JsValue::Undefined(), JsValue::Undefined() };
  args[1] = j.dump();
  return parse.Call(args);
}
}

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

void DynamicFields::Set(const std::string& propName, const JsValue& value)
{
  pImpl->jsonCache.reset();
  pImpl->props[propName] = JsValueToJson(value);
}

JsValue DynamicFields::Get(const std::string& propName) const
{
  auto it = pImpl->props.find(propName);
  if (it == pImpl->props.end()) {
    return JsValue::Undefined();
  }
  return JsonToJsValue(it->second);
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
    res.Set(it.key(), JsonToJsValue(it.value()));
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
