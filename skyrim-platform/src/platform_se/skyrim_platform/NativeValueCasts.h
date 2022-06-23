#pragma once
#include "CallNative.h"

namespace NativeValueCasts {

CallNative::ObjectPtr JsObjectToNativeObject(const JsValue& v);
JsValue NativeObjectToJsObject(const CallNative::ObjectPtr& obj);
CallNative::AnySafe JsValueToNativeValue(const JsValue& v);
JsValue NativeValueToJsValue(const CallNative::AnySafe& v);
}
