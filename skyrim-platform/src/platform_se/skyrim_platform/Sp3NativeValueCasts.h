#pragma once

#include "CallNative.h"

class Sp3NativeValueCasts
{
public:
    static CallNative::ObjectPtr JsObjectToNativeObject(const Napi::Value& v);
    static Napi::Value NativeObjectToJsObject(Napi::Env env,
                                   const CallNative::ObjectPtr& obj);
    static CallNative::AnySafe JsValueToNativeValue(const Napi::Value& v);
    static Napi::Value NativeValueToJsValue(Napi::Env env, const CallNative::AnySafe& v);
};
