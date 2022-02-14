#pragma once
#include "NativeValueCasts.h"

inline JsValue CreateObject(const char* type, void* form)
{
  return form ? NativeValueCasts::NativeObjectToJsObject(
                  std::make_shared<CallNative::Object>(type, form))
              : JsValue::Null();
}

inline void AddObjProperty(JsValue* obj, const char* tag, const char* property)
{
  obj->SetProperty(tag, JsValue::String(property));
}

inline void AddObjProperty(JsValue* obj, const char* tag,
                           const std::string& property)
{
  obj->SetProperty(tag, JsValue::String(property));
}

inline void AddObjProperty(JsValue* obj, const char* tag,
                           const FixedString& property)
{
  obj->SetProperty(tag, JsValue::String(property.c_str()));
}

inline void AddObjProperty(JsValue* obj, const char* tag, bool property)
{
  obj->SetProperty(tag, JsValue::Bool(property));
}

inline void AddObjProperty(JsValue* obj, const char* tag, int property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

inline void AddObjProperty(JsValue* obj, const char* tag, uint16_t property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

inline void AddObjProperty(JsValue* obj, const char* tag, uint32_t property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

inline void AddObjProperty(JsValue* obj, const char* tag, uintptr_t property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

inline void AddObjProperty(JsValue* obj, const char* tag, float property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

inline void AddObjProperty(JsValue* obj, const char* tag, const uint8_t* data,
                           uint32_t length)
{
  auto typedArray = JsValue::Uint8Array(length);
  memcpy(typedArray.GetTypedArrayData(), data, length);
  obj->SetProperty(tag, typedArray);
}

inline void AddObjProperty(JsValue* obj, const char* tag,
                           RE::TESForm* property, const char* typeName)
{
  if (property) {
    obj->SetProperty(tag,
                     CreateObject(typeName, static_cast<void*>(property)));
  } else {
    obj->SetProperty(tag, JsValue::Undefined());
  }
}
