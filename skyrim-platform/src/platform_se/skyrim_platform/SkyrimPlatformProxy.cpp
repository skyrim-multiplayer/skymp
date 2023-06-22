#include "SkyrimPlatformProxy.h"
#include "JsUtils.h"
#include "NativeValueCasts.h"
#include "ProxyGetter.h"
#include "VmProvider.h"
#include "Override.h"
#include "NullPointerException.h"
#include "CallNativeApi.h"
#include "CallNative.h"
#include "CreatePromise.h"
using namespace CallNativeApi;

namespace {

JsValue CallNative(
  const std::vector<JsValue>& args,
  const std::function<NativeCallRequirements()>& getNativeCallRequirements)
{
  auto className = (std::string)args[1];
  auto functionName = (std::string)args[2];
  auto self = args[3];
  constexpr int nativeArgsStart = 4;

  auto requirements = getNativeCallRequirements();
  if (!requirements.vm)
    throw std::runtime_error('\'' + className + '.' + functionName +
                             "' can't be called in this context");

  CallNative::AnySafe nativeArgs[CallNative::g_maxArgs + 1];
  auto n = (size_t)std::max((int)args.size() - nativeArgsStart, 0);

  for (size_t i = 0; i < n; ++i)
    nativeArgs[i] =
      NativeValueCasts::JsValueToNativeValue(args[nativeArgsStart + i]);

  static VmProvider provider;

  CallNative::Arguments callNativeArgs{
    requirements.vm,
    requirements.stackId,
    className,
    functionName,
    NativeValueCasts::JsObjectToNativeObject(self),
    nativeArgs,
    n,
    provider,
    *requirements.gameThrQ,
    *requirements.jsThrQ,
    nullptr
  };

  auto f = provider.GetFunctionInfo(className, functionName);
  auto isAddOrRemove =
    (functionName == "removeItem") || (functionName == "addItem");

  if (f && f->IsLatent() && !isAddOrRemove) {

    thread_local CallNative::Arguments* g_callNativeArgsPtr = nullptr;
    g_callNativeArgsPtr = &callNativeArgs;

    thread_local auto g_promiseFn =
      JsValue::Function([](const JsFunctionArguments& args) {
        auto resolve = std::shared_ptr<JsValue>(new JsValue(args[1]));
        if (!g_callNativeArgsPtr)
          throw NullPointerException("g_callNativeArgsPtr");
        g_callNativeArgsPtr->latentCallback =
          [resolve](const CallNative::AnySafe& v) {
            resolve->Call({ JsValue::Undefined(),
                            NativeValueCasts::NativeValueToJsValue(v) });
          };
        CallNative::CallNativeSafe(*g_callNativeArgsPtr);
        return JsValue::Undefined();
      });
    return CreatePromise(g_promiseFn);
  } else {
    Override o;
    auto res = NativeValueCasts::NativeValueToJsValue(
      CallNative::CallNativeSafe(callNativeArgs));
    return res;
  }
}

JsValue GetProxyForClass(const std::string& className, const JsValue& skyrimPlatformExports,
    std::function<NativeCallRequirements()> getNativeCallRequirements)
{
  std::shared_ptr<std::unordered_map<std::string, JsValue>> functionsCache(
    new std::unordered_map<std::string, JsValue>);

  bool isGame = !stricmp(className.data(), "Game");
  bool isObjectReference = !stricmp(className.data(), "ObjectReference");
  bool isActor = !stricmp(className.data(), "Actor");

  auto handler = JsValue::Object();
  handler.SetProperty(
    "get", ProxyGetter([=](const JsValue& origin, const JsValue& keyStr) {
      auto s = (std::string)keyStr;
      auto& f = (*functionsCache)[s];

      if (f.GetType() != JsValue::Type::Function) {

        std::shared_ptr<std::vector<JsValue>> callNativeArgs(
          new std::vector<JsValue>{ origin, className, keyStr,
                                    JsValue::Null() /*self*/ });
        std::shared_ptr<std::vector<JsValue>> dynamicCastArgs(
          new std::vector<JsValue>{ origin, JsValue::Undefined(), className });

        if (isGame && s == "getFormEx") {
          f =
            JsValue::Function([](const JsFunctionArguments& args) -> JsValue {
              auto f = RE::TESForm::LookupByID((uint32_t)(double)args[1]);
              if (f)
                return CreateObject("Form", f);
              else
                return JsValue::Null();
            });
        } else if (s == "from") {
          if (isObjectReference) {
            f = JsValue::Function(
              [](const JsFunctionArguments& args) -> JsValue {
                CallNative::ObjectPtr obj =
                  NativeValueCasts::JsObjectToNativeObject(args[1]);
                if (!obj)
                  return JsValue::Null();
                auto form = (RE::TESForm*)obj->GetNativeObjectPtr();
                if (!form)
                  return JsValue::Null();
                auto objRefr = form->As<RE::TESObjectREFR>();
                if (!objRefr)
                  return JsValue::Null();
                return CreateObject("ObjectReference", objRefr);
              });
          } else if (isActor) {
            f = JsValue::Function(
              [](const JsFunctionArguments& args) -> JsValue {
                CallNative::ObjectPtr obj =
                  NativeValueCasts::JsObjectToNativeObject(args[1]);
                if (!obj)
                  return JsValue::Null();
                auto form = (RE::TESForm*)obj->GetNativeObjectPtr();
                if (!form)
                  return JsValue::Null();
                auto actor = form->As<RE::Actor>();
                if (!actor)
                  return JsValue::Null();
                return CreateObject("Actor", actor);
              });
          } else {
            f = JsValue::Function(
              [keyStr, origin, className, callNativeArgs,
               dynamicCastArgs](const JsFunctionArguments& args) -> JsValue {
                auto& from = args[1];
                (*dynamicCastArgs)[1] = from;
                thread_local auto dynamicCast =
                  origin.GetProperty("dynamicCast");
                return dynamicCast.Call(*dynamicCastArgs);
              });
          }
        } else {
          f = JsValue::Function([keyStr, className, callNativeArgs, getNativeCallRequirements](
                                  const JsFunctionArguments& args) -> JsValue {
            callNativeArgs->resize(args.GetSize() + 3);
            for (size_t i = 1; i < args.GetSize(); ++i)
              (*callNativeArgs)[i + 3] = args[i];
            return CallNative(*callNativeArgs, getNativeCallRequirements);
          });
        }
      }
      return f;
    }));
  return JsValue::GlobalObject().GetProperty("Proxy").Constructor(
    { JsValue::Undefined(), skyrimPlatformExports, handler });
}
}

JsValue SkyrimPlatformProxy::Attach(const JsValue& skyrimPlatformExports,
  std::function<CallNativeApi::NativeCallRequirements()> getNativeCallRequirements)
{
  thread_local std::unordered_map<std::string, JsValue> g_classProxies;

  auto handler = JsValue::Object();
  handler.SetProperty(
    "get", ProxyGetter([=](const JsValue& origin, const JsValue& keyStr) {
      auto& proxy = g_classProxies[(std::string)keyStr];
      if (proxy.GetType() != JsValue::Type::Object)
        proxy = GetProxyForClass((std::string)keyStr, skyrimPlatformExports, getNativeCallRequirements);
      return proxy;
    }));
  return JsValue::GlobalObject().GetProperty("Proxy").Constructor(
    { JsValue::Undefined(), skyrimPlatformExports, handler });
}
