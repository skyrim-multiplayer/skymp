#include "DynamicFields.h"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#include <optional>

namespace {
nlohmann::json JsValueToJson(Napi::Env env, const Napi::Value& value)
{
  auto builtinJson = env.Global().Get("JSON").As<Napi::Object>();
  auto builtinStringify = builtinJson.Get("stringify").As<Napi::Function>();
  auto result = builtinStringify.Call(builtinJson, { value });
  auto dump = static_cast<std::string>(result.As<Napi::String>());
  return nlohmann::json::parse(dump);
}

Napi::Value JsonToJsValue(Napi::Env env, const nlohmann::json& j)
{
  auto builtinJson = env.Global().Get("JSON").As<Napi::Object>();
  auto builtinParse = builtinJson.Get("parse").As<Napi::Function>();
  auto result = builtinParse.Call(builtinJson, { Napi::String::New(env, j.dump()) });
  return result;
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

void DynamicFields::Set(Napi::Env env, const std::string& propName, const Napi::Value& value)
{
  pImpl->jsonCache.reset();
  pImpl->props[propName] = JsValueToJson(env, value);
}

Napi::Value DynamicFields::Get(Napi::Env env, const std::string& propName) const
{
  auto it = pImpl->props.find(propName);
  if (it == pImpl->props.end()) {
    return env.Undefined();
  }
  return JsonToJsValue(env, it->second);
}

void DynamicFields::Set(const std::string& propName, const nlohmann::json& value)
{
  pImpl->jsonCache.reset();
  pImpl->props[propName] = value;
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
