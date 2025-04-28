#include "SP3NativeValueCasts.h"

#include "CallNative.h"
#include "NullPointerException.h"
#include "Overloaded.h"
#include "PapyrusTESModPlatform.h"
#include <optional>
#include <spdlog/spdlog.h>
#include <vector>

struct SP3NativeValueCasts::Impl
{
  std::optional<uint64_t> numPapyrusUpdates;
  std::vector<CallNative::ObjectPtr> pool;
};

SP3NativeValueCasts& SP3NativeValueCasts::GetSingleton()
{
  static SP3NativeValueCasts g_instance;
  return g_instance;
}

CallNative::ObjectPtr SP3NativeValueCasts::JsObjectToNativeObject(
  const Napi::Value& v)
{
  if (v.IsNull() || v.IsUndefined()) {
    return nullptr;
  }

  if (!v.IsObject()) {
    throw std::runtime_error(
      "JsObjectToNativeObject expected object or null or undefined");
  }

  auto indexInPool =
    v.As<Napi::Object>()
      .Get(SP3NativeValueCasts::kSkyrimPlatformIndexInPoolProperty)
      .ToNumber()
      .Int64Value();

  if (indexInPool < 0 || indexInPool >= pImpl->pool.size()) {
    spdlog::error("SP3NativeValueCasts::JsObjectToNativeObject - Invalid "
                  "index in pool: {}",
                  indexInPool);
    return nullptr;
  }

  return pImpl->pool[indexInPool];
}

Napi::Value SP3NativeValueCasts::NativeObjectToJsObject(
  Napi::Env env, const CallNative::ObjectPtr& obj)
{
  if (!obj) {
    return env.Null();
  }

  const auto numPapyrusUpdates = TESModPlatform::GetNumPapyrusUpdates();

  if (pImpl->numPapyrusUpdates != numPapyrusUpdates) {
    pImpl->numPapyrusUpdates = numPapyrusUpdates;
    pImpl->pool.clear();
  }

  auto equals = [&obj](const CallNative::ObjectPtr& poolObj) {
    return obj == poolObj ||
      (obj->GetType() == poolObj->GetType() &&
       obj->GetNativeObjectPtr() == poolObj->GetNativeObjectPtr());
  };

  auto it = std::find_if(pImpl->pool.begin(), pImpl->pool.end(), equals);

  if (it == pImpl->pool.end()) {
    it = pImpl->pool.insert(pImpl->pool.end(), obj);
  }

  auto distance = std::distance(pImpl->pool.begin(), it);
  auto result = Napi::Object::New(env);
  result.Set(SP3NativeValueCasts::kSkyrimPlatformIndexInPoolProperty,
             Napi::Number::New(env, distance));
  return result;
}

CallNative::AnySafe SP3NativeValueCasts::JsValueToNativeValue(
  const Napi::Value& v)
{
  // TODO: better switch(v.Type()) for performance
  if (v.IsBoolean()) {
    return static_cast<bool>(v.As<Napi::Boolean>());
  }

  if (v.IsNumber()) {
    return static_cast<double>(v.As<Napi::Number>().DoubleValue());
  }

  if (v.IsString()) {
    return static_cast<std::string>(v.As<Napi::String>());
  }

  if (v.IsUndefined() || v.IsNull() || v.IsObject()) {
    return JsObjectToNativeObject(v);
  }

  throw std::runtime_error("Unsupported JavaScript type (" +
                           std::to_string((int)v.Type()) + ")");
}

Napi::Value SP3NativeValueCasts::NativeValueToJsValue(
  Napi::Env env, const CallNative::AnySafe& v)
{
  if (v.valueless_by_exception()) {
    return env.Null();
  }
  return std::visit(
    overloaded{
      [env](double v) -> Napi::Value { return Napi::Number::New(env, v); },
      [env](bool v) -> Napi::Value { return Napi::Boolean::New(env, v); },
      [env](const std::string& v) -> Napi::Value {
        return Napi::String::New(env, v);
      },
      [env, this](const CallNative::ObjectPtr& v) -> Napi::Value {
        return NativeObjectToJsObject(env, v);
      },
      [env](const std::vector<std::string>& v) -> Napi::Value {
        auto out = Napi::Array::New(env, v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.Set(i, Napi::String::New(env, v[i]));
        }
        return out;
      },
      [env](const std::vector<bool>& v) -> Napi::Value {
        auto out = Napi::Array::New(env, v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.Set(i, Napi::Boolean::New(env, v[i]));
        }
        return out;
      },
      [env](const std::vector<double>& v) -> Napi::Value {
        auto out = Napi::Array::New(env, v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.Set(i, Napi::Number::New(env, v[i]));
        }
        return out;
      },
      [env, this](const std::vector<CallNative::ObjectPtr>& v) -> Napi::Value {
        auto out = Napi::Array::New(env, v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.Set(i, NativeObjectToJsObject(env, v[i]));
        }
        return out;
      },
    },
    v);
}

SP3NativeValueCasts::SP3NativeValueCasts()
{
  pImpl = std::make_shared<Impl>();
}
