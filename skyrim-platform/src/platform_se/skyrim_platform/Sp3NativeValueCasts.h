#pragma once

#include "CallNative.h"
#include <memory>

class SP3NativeValueCasts
{
public:
  constexpr static auto kSkyrimPlatformIndexInPoolProperty =
    "_skyrimPlatform_indexInPool";

  static SP3NativeValueCasts& GetSingleton();

  CallNative::ObjectPtr JsObjectToNativeObject(const Napi::Value& v);
  Napi::Value NativeObjectToJsObject(Napi::Env env,
                                     const CallNative::ObjectPtr& obj);
  CallNative::AnySafe JsValueToNativeValue(const Napi::Value& v);
  Napi::Value NativeValueToJsValue(Napi::Env env,
                                   const CallNative::AnySafe& v);

private:
  SP3NativeValueCasts();

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
