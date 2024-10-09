#pragma once
#include "CallNative.h"

#include "NapiHelper.h"

namespace NativeValueCasts {

CallNative::ObjectPtr JsObjectToNativeObject(const Napi::Value& v);
Napi::Value NativeObjectToJsObject(Napi::Env env,
                                   const CallNative::ObjectPtr& obj);
CallNative::AnySafe JsValueToNativeValue(const Napi::Value& v);
Napi::Value NativeValueToJsValue(Napi::Env env, const CallNative::AnySafe& v);
}
