#include "CallNativeApi.h"

#include "CallNative.h"
#include "NativeObjectProxy.h"
#include "NullPointerException.h"
#include "Overloaded.h"
#include "PapyrusTESModPlatform.h"
#include "VmProvider.h"

namespace {
class NativeObject : public JsExternalObjectBase
{
public:
  NativeObject(const CallNative::ObjectPtr& obj_)
    : papyrusUpdateId(TESModPlatform::GetNumPapyrusUpdates())
    , obj(obj_)
  {
  }

  const CallNative::ObjectPtr& Get() const
  {
    auto n = TESModPlatform::GetNumPapyrusUpdates();
    if (n != papyrusUpdateId) {
      std::stringstream ss;
      ss << "This game object is expired, consider saving form ID instead of "
            "the object itself (papyrusUpdateId="
         << papyrusUpdateId << ", n=" << n << ")";
      throw std::runtime_error(ss.str());
    }
    return obj;
  }

  uint64_t papyrusUpdateId;
  CallNative::ObjectPtr obj;
};

CallNative::ObjectPtr JsObjectToNativeObject(const JsValue& v)
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

JsValue ToString(const JsFunctionArguments& args)
{
  auto nativeObj = JsObjectToNativeObject(args[0]);
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

JsValue NativeObjectToJsObject(const CallNative::ObjectPtr& obj)
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
    NativeObjectProxy::Attach(poolEntry.object, obj->GetType(), g_toString);
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

CallNative::AnySafe JsValueToNativeValue(const JsValue& v)
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

JsValue NativeValueToJsValue(const CallNative::AnySafe& v)
{
  return std::visit(
    overloaded{ [](double v) { return JsValue(v); },
                [](bool v) { return JsValue::Bool(v); },
                [](const std::string& v) { return JsValue(v); },
                [](const CallNative::ObjectPtr& v) {
                  return NativeObjectToJsObject(v);
                } },
    v);
}
}

JsValue CallNativeApi::CallNative(
  const JsFunctionArguments& args,
  const std::function<NativeCallRequirements()>& getNativeCallRequirements)
{
  auto className = (std::string)args[1];
  auto functionName = (std::string)args[2];
  auto self = args[3];
  constexpr int nativeArgsStart = 4;

  auto requirements = getNativeCallRequirements();
  if (!requirements.vm)
    throw std::runtime_error("CallNative can't be called in this context");

  CallNative::AnySafe nativeArgs[CallNative::g_maxArgs + 1];
  auto n = (size_t)std::max((int)args.GetSize() - nativeArgsStart, 0);

  for (size_t i = 0; i < n; ++i)
    nativeArgs[i] = JsValueToNativeValue(args[nativeArgsStart + i]);

  static VmProvider provider;

  auto res = CallNative::CallNativeSafe(
    requirements.vm, requirements.stackId, className, functionName,
    JsValueToNativeValue(self), nativeArgs, n, provider);

  return NativeValueToJsValue(res);
}

JsValue CallNativeApi::DynamicCast(
  const JsFunctionArguments& args,
  const std::function<NativeCallRequirements()>& getNativeCallRequirements)
{
  auto form = JsValueToNativeValue(args[1]);
  auto targetType = std::string(args[2]);
  return NativeValueToJsValue(CallNative::DynamicCast(targetType, form));
}