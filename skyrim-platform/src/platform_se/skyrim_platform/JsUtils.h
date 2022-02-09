#pragma once
#include "NativeValueCasts.h"

JsValue CreateObject(const char* type, void* form)
{
  return form ? NativeValueCasts::NativeObjectToJsObject(
                  std::make_shared<CallNative::Object>(type, form))
              : JsValue::Null();
}

void AddObjProperty(JsValue* obj, const char* tag, const char* property)
{
  obj->SetProperty(tag, JsValue::String(property));
}

void AddObjProperty(JsValue* obj, const char* tag, std::string property)
{
  obj->SetProperty(tag, JsValue::String(property));
}

void AddObjProperty(JsValue* obj, const char* tag, std::string_view property)
{
  obj->SetProperty(tag, JsValue::String(std::string{ property }));
}

void AddObjProperty(JsValue* obj, const char* tag, bool property)
{
  obj->SetProperty(tag, JsValue::Bool(property));
}

void AddObjProperty(JsValue* obj, const char* tag, int property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

void AddObjProperty(JsValue* obj, const char* tag, uint32_t property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

void AddObjProperty(JsValue* obj, const char* tag, uintptr_t property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

void AddObjProperty(JsValue* obj, const char* tag, float property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

void AddObjProperty(JsValue* obj, const char* tag, const uint8_t* data,
                    uint32_t length)
{
  auto typedArray = JsValue::Uint8Array(length);
  memcpy(typedArray.GetTypedArrayData(), data, length);
  obj->SetProperty(tag, typedArray);
}

void AddObjProperty(JsValue* obj, const char* tag, RE::TESForm* property,
                    const char* typeName)
{
  obj->SetProperty(tag, CreateObject(typeName, static_cast<void*>(property)));
}

void AddObjProperty(JsValue* obj, const char* tag, RE::ActiveEffect* property,
                    const char* typeName)
{
  obj->SetProperty(tag, CreateObject(typeName, static_cast<void*>(property)));
}
