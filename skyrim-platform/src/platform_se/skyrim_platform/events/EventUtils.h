#pragma once

#include "../NativeValueCasts.h"

/**
 * concepts to distinguish what approach we should take to register sinks
 */
template <class T>
concept HasEvent = requires
{
  typename T::Event;
};

template <class T, class E>
concept SingletonSource = requires
{
  {
    T::GetSingleton()
    } -> std::convertible_to<RE::BSTEventSource<E>*>;
};

/**
 * @brief Create event names vector.
 */
std::vector<const char*>* CreateEventList(
  std::initializer_list<const char*> list)
{
  auto v = std::vector(list);
  return &v;
}

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

void AddProperty(JsValue* obj, const char* tag, std::string property)
{
  obj->SetProperty(tag, JsValue::String(property));
}

void AddProperty(JsValue* obj, const char* tag, std::string_view property)
{
  obj->SetProperty(tag, JsValue::String(std::string{ property }));
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

void AddProperty(JsValue* obj, const char* tag, uintptr_t property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

void AddProperty(JsValue* obj, const char* tag, float property)
{
  obj->SetProperty(tag, JsValue::Double(property));
}

void AddProperty(JsValue* obj, const char* tag, const uint8_t* data,
                 uint32_t length)
{
  auto typedArray = JsValue::Uint8Array(length);
  memcpy(typedArray.GetTypedArrayData(), data, length);
  obj->SetProperty(tag, typedArray);
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
