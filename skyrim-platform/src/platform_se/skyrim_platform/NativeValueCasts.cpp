#include "NativeValueCasts.h"
#include "NativeObject.h"
#include "NativeObjectProxy.h"
#include "NullPointerException.h"
#include "Overloaded.h"
#include "PapyrusTESModPlatform.h"

namespace {
JsValue ToString(const JsFunctionArguments& args)
{
  auto nativeObj = NativeValueCasts::JsObjectToNativeObject(args[0]);
  std::stringstream ss;
  ss << "[object " << (nativeObj ? nativeObj->GetType() : "") << "]";
  return ss.str();
}

struct PoolEntry
{
  JsValue object;
  const char* type = "";
  NativeObject* nativeObject = nullptr;
};
}

CallNative::ObjectPtr NativeValueCasts::JsObjectToNativeObject(
  const JsValue& v)
{
  switch (v.GetType()) {
    case JsValue::Type::Null:
    case JsValue::Type::Undefined:
      return nullptr;
    case JsValue::Type::Object: {
      auto nativeObj = dynamic_cast<NativeObject*>(v.GetExternalData());
      if (!nativeObj)
        throw std::runtime_error(
          "This JavaScript object is not a valid game object");
      return nativeObj->Get();
    }
    default:
      throw std::runtime_error("Unsupported JavaScript type (" +
                               std::to_string((int)v.GetType()) + ")");
  }
}

JsValue NativeValueCasts::NativeObjectToJsObject(
  const CallNative::ObjectPtr& obj)
{
  if (!obj)
    return JsValue::Null();

  const auto numPapyrusUpdates = TESModPlatform::GetNumPapyrusUpdates();

  thread_local auto g_toString = JsValue::Function(ToString);
  thread_local std::unordered_map<void*, PoolEntry>
    g_nativeObjectPool; // TODO: Add memory usage metrics for this

  auto nativeObjPtr = obj->GetNativeObjectPtr();
  if (!nativeObjPtr)
    throw NullPointerException("nativeObjPtr");

  auto& poolEntry = g_nativeObjectPool[nativeObjPtr];
  if (poolEntry.object.GetType() != JsValue::Type::Object ||
      0 != strcmp(poolEntry.type, obj->GetType())) {
    auto nativeObject = new NativeObject(obj);
    poolEntry.object = JsValue::ExternalObject(nativeObject);
    poolEntry.type = obj->GetType();
    poolEntry.nativeObject = nativeObject;
    thread_local auto g_toJson =
      JsValue::Null(); // Called by JSON.stringify if callable
    NativeObjectProxy::Attach(poolEntry.object, obj->GetType(), g_toString,
                              g_toJson);
  }
  poolEntry.nativeObject->papyrusUpdateId = numPapyrusUpdates;

  // Clear pool
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

CallNative::AnySafe NativeValueCasts::JsValueToNativeValue(const JsValue& v)
{
  switch (v.GetType()) {
    case JsValue::Type::Boolean:
      return (bool)v;
    case JsValue::Type::Number:
      return (double)v;
    case JsValue::Type::String:
      return (std::string)v;
    case JsValue::Type::Object:
    case JsValue::Type::Null:
    case JsValue::Type::Undefined:
      return JsObjectToNativeObject(v);
    default:
      throw std::runtime_error("Unsupported JavaScript type (" +
                               std::to_string((int)v.GetType()) + ")");
  }
}

JsValue NativeValueCasts::NativeValueToJsValue(const CallNative::AnySafe& v)
{
  if (v.valueless_by_exception())
    return JsValue::Null();
  return std::visit(
    overloaded{
      [](double v) { return JsValue(v); },
      [](bool v) { return JsValue::Bool(v); },
      [](const std::string& v) { return JsValue(v); },
      [](const CallNative::ObjectPtr& v) { return NativeObjectToJsObject(v); },
      [](const std::vector<std::string>& v) {
        auto out = JsValue::Array(v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.SetProperty(JsValue::Int(i), v[i]);
        }
        return out;
      },
      [](const std::vector<bool>& v) {
        auto out = JsValue::Array(v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.SetProperty(JsValue::Int(i), JsValue::Bool(v[i]));
        }
        return out;
      },
      [](const std::vector<double>& v) {
        auto out = JsValue::Array(v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.SetProperty(JsValue::Int(i), v[i]);
        }
        return out;
      },
      [](const std::vector<CallNative::ObjectPtr>& v) {
        auto out = JsValue::Array(v.size());
        for (size_t i = 0; i < v.size(); ++i) {
          out.SetProperty(JsValue::Int(i), NativeObjectToJsObject(v[i]));
        }
        return out;
      },
    },
    v);
}
