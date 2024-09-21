#pragma once

#include "NapiHelper.h"

using ProxyGetterFn =
  std::function<Napi::Value(const Napi::Object& origin, const Napi::String& keyStr)>;

inline Napi::Value ProxyGetter(Napi::Env env, const ProxyGetterFn& f)
{
  return Napi::Function::New(env, NapiHelper::WrapCppExceptions([=](const Napi::CallbackInfo &info) -> Napi::Value {
    auto& origin = Napi::ExtractObject(info[0], "origin");
    auto& key = Napi::ExtractString(info[1], "key");
    auto originProperty = origin.Get(key);
    if (!originProperty.IsUndefined()) {
      return originProperty;
    }
    return (!key.IsString()) ? info.Env().Undefined() : f(origin, key);
  }));
}
