#pragma once
#include "JsEngine.h"

inline Napi::Value CreatePromise(const Napi::Value &resolver)
{
  Napi::Env env = resolver.Env();
  Napi::Value standardPromise = env.Global().Get("Promise");
  Napi::Object promiseInstance = standardPromise.As<Napi::Function>().New({ resolver });
  return promiseInstance;
}
