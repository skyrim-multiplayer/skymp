#include "NativeValueCasts.h"
#include "NativeObject.h"
#include "NativeObjectProxy.h"
#include "NullPointerException.h"
#include "Overloaded.h"
#include "PapyrusTESModPlatform.h"

namespace {
Napi::Value ToString(const Napi::CallbackInfo& info)
{
  auto nativeObj = NativeValueCasts::JsObjectToNativeObject(info[0]);
  std::stringstream ss;
  ss << "[object " << (nativeObj ? nativeObj->GetType() : "") << "]";
  return ss.str();
}

struct PoolEntry
{
  std::shared_ptr<Napi::Reference<Napi::Object>> object;
  const char* type = "";
  NativeObject* nativeObject = nullptr;
};
}

CallNative::ObjectPtr NativeValueCasts::JsObjectToNativeObject(
  const Napi::Value& v)
{
  if (v.IsNull() || v.IsUndefined()) {
    return nullptr;
  }

  // Check if the value is an external object
  if (v.IsExternal()) {
    Napi::External<NativeObject> ext = v.As<Napi::External<NativeObject>>();
    NativeObject* nativeObj = ext.Data();

    if (nativeObj) {
      return nativeObj;
    } else {
      throw std::runtime_error(
        "This JavaScript object does not contain a valid native object");
    }
  } else if (v.IsObject()) {
    throw std::runtime_error(
      "This JavaScript object is not a valid native object");
  } else {
    throw std::runtime_error(
      "Unsupported JavaScript type (not an object or external)");
  }
}


Napi::Value NativeValueCasts::NativeObjectToJsObject(Napi::Env env,
  const CallNative::ObjectPtr& obj)
{
  if (!obj) {
    return env.Null();
  }

  const auto numPapyrusUpdates = TESModPlatform::GetNumPapyrusUpdates();

  auto toString = Napi::Function::New(env, ToString);
  thread_local std::unordered_map<void*, PoolEntry>
    g_nativeObjectPool; // TODO: Add memory usage metrics for this

  auto nativeObjPtr = obj->GetNativeObjectPtr();
  if (!nativeObjPtr)
    throw NullPointerException("nativeObjPtr");

  auto& poolEntry = g_nativeObjectPool[nativeObjPtr];
  if (!poolEntry.object || 0 != strcmp(poolEntry.type, obj->GetType())) {
    auto nativeObject = new NativeObject(obj);
    auto newExternal = Napi::External<NativeObject>::New(env, nativeObject);
    poolEntry.object.reset(new Napi::Reference(Napi::Persistent(newExternal)));
    poolEntry.type = obj->GetType();
    poolEntry.nativeObject = nativeObject;
    auto toJson =
      env.Null(); // Called by JSON.stringify if callable
    NativeObjectProxy::Attach(newExternal, obj->GetType(), toString,
                              toJson);
  }
  poolEntry.nativeObject->papyrusUpdateId = numPapyrusUpdates;

  // Clean pool from time to time
  thread_local uint32_t g_callCounter = 0;
  ++g_callCounter;
  if (g_callCounter == 1000) {
    g_callCounter = 0;

    thread_local std::vector<void*> g_tmpKeysToErase;
    g_tmpKeysToErase.clear();

    for (auto& [objRawPtr, poolEntry] : g_nativeObjectPool) {
      if (numPapyrusUpdates - poolEntry.nativeObject->papyrusUpdateId > 60)
        g_tmpKeysToErase.push_back(objRawPtr);
    }
    for (auto k : g_tmpKeysToErase)
      g_nativeObjectPool.erase(k);
  }

  return poolEntry.object;
}

CallNative::AnySafe NativeValueCasts::JsValueToNativeValue(const Napi::Value& v)
{
  // TODO: better switch(v.Type())
  if (v.IsBoolean()) {
    return static_cast<bool>(v.As<Napi::Boolean>());
  }

  if (v.IsNumber()) {
    return static_cast<double>(v.As<Napi::Number>().DoubleValue());
  }

  if (v.IsString()) {
    return static_cast<std::string>(v.As<Napi::String>());
  }

  if (v.IsUndefined() || v.IsNull() || v.IsObject()) {
    return JsObjectToNativeObject(v);
  }

  throw std::runtime_error("Unsupported JavaScript type (" +
                            std::to_string((int)v.Type()) + ")");
}

Napi::Value NativeValueCasts::NativeValueToJsValue(Napi::Env env, const CallNative::AnySafe& v)
{
  if (v.valueless_by_exception()) {
    return env.Null();
  }
  return std::visit(
    overloaded{
      [env](double v) { return Napi::Number::New(env, v); },
      [env](bool v) { return Napi::Boolean::New(env, v); },
      [env](const std::string& v) { return Napi::String::New(env, v); },
      [env](const CallNative::ObjectPtr& v) { return NativeObjectToJsObject(env, v); },
      [env](const std::vector<std::string>& v) {
        auto out = Napi::Array::New(env, v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.Set(i, Napi::String::New(env, v[i]));
        }
        return out;
      },
      [env](const std::vector<bool>& v) {
        auto out = Napi::Array::New(env, v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.Set(i, Napi::Boolean::New(env, v[i]));
        }
        return out;
      },
      [env](const std::vector<double>& v) {
        auto out = Napi::Array::New(env, v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.Set(i, Napi::Number::New(env, v[i]));
        }
        return out;
      },
      [env](const std::vector<CallNative::ObjectPtr>& v) {
        auto out = Napi::Array::New(env, v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.Set(i, NativeObjectToJsObject(v[i]));
        }
        return out;
      },
    },
    v);
}
