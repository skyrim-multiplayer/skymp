#include "NativeValueCasts.h"

JsValue CreateObject(const char* type, void* form)
{
  return form ? NativeValueCasts::NativeObjectToJsObject(
                  std::make_shared<CallNative::Object>(type, form))
              : JsValue::Null();
}

void AddProperty(JsValue* obj, const char* tag, const char* property)
{
  obj->SetProperty(tag, JsValue::String(property));
}

void AddProperty(JsValue* obj, const char* tag, bool property)
{
  obj->SetProperty(tag, JsValue::Bool(property));
}

void AddProperty(JsValue* obj, const char* tag, int property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

void AddProperty(JsValue* obj, const char* tag, uint32_t property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

void AddProperty(JsValue* obj, const char* tag, float property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

void AddProperty(JsValue* obj, const char* tag, RE::TESForm* property,
                 const char* typeName)
{
  obj->SetProperty(tag, CreateObject(typeName, static_cast<void*>(property)));
}

void AddProperty(JsValue* obj, const char* tag, RE::ActiveEffect* property,
                 const char* typeName)
{
  obj->SetProperty(tag, CreateObject(typeName, static_cast<void*>(property)));
}
