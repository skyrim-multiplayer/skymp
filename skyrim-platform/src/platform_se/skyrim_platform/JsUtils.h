#pragma once
#include "NativeValueCasts.h"

inline Napi::Value CreateObject(Napi::Env env, const char* type, void* form)
{
  return form ? NativeValueCasts::NativeObjectToJsObject(
                  env, std::make_shared<CallNative::Object>(type, form))
              : env.Null();
}

inline void AddObjProperty(Napi::Object* obj, const char* tag, const char* property)
{
  obj->Set(tag, Napi::String::New(obj->Env(), property));
}

inline void AddObjProperty(Napi::Object* obj, const char* tag,
                           const std::string& property)
{
  obj->Set(tag, Napi::String::New(obj->Env(), property));
}

inline void AddObjProperty(Napi::Object* obj, const char* tag,
                           const FixedString& property)
{
  obj->Set(tag, Napi::String::New(obj->Env(), property.c_str()));
}

inline void AddObjProperty(Napi::Object* obj, const char* tag, bool property)
{
  obj->Set(tag, Napi::Boolean::New(obj->Env(), property));
}

inline void AddObjProperty(Napi::Object* obj, const char* tag, int property)
{
  obj->Set(tag, Napi::Number::New(obj->Env(), property));
}

inline void AddObjProperty(Napi::Object* obj, const char* tag, uint16_t property)
{
  obj->Set(tag, Napi::Number::New(obj->Env(), property));
}

inline void AddObjProperty(Napi::Object* obj, const char* tag, uint32_t property)
{
  obj->Set(tag, Napi::Number::New(obj->Env(), property));
}

inline void AddObjProperty(Napi::Object* obj, const char* tag, uintptr_t property)
{
  obj->Set(tag, Napi::Number::New(obj->Env(), property));
}

inline void AddObjProperty(Napi::Object* obj, const char* tag, float property)
{
  obj->Set(tag, Napi::Number::New(obj->Env(), property));
}

inline void AddObjProperty(Napi::Object* obj, const char* tag, const uint8_t* data,
                           uint32_t length)
{
  auto typedArray = Napi::Uint8Array::New(obj->Env(), length);
  memcpy(typedArray.Data(), data, length);
  obj->Set(tag, typedArray);
}

inline void AddObjProperty(Napi::Object* obj, const char* tag,
                           RE::TESForm* property, const char* typeName)
{
  if (property) {
    obj->Set(tag,
                     CreateObject(obj->Env(), typeName, static_cast<void*>(property)));
  } else {
    obj->Set(tag, obj->Env().Undefined());
  }
}
