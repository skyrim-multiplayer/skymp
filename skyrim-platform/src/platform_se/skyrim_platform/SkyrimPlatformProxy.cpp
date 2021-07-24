#include "SkyrimPlatformProxy.h"
#include "NativeValueCasts.h"
#include "ProxyGetter.h"
#include <skse64/GameRTTI.h>
#include <skse64/GameReferences.h>
#include <unordered_map>

namespace {
JsValue CreateObject(const char* type, void* form)
{
  return form ? NativeValueCasts::NativeObjectToJsObject(
                  std::make_shared<CallNative::Object>(type, form))
              : JsValue::Null();
}

JsValue GetProxyForClass(const std::string& className,
                         const JsValue& skyrimPlatformExports)
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
              auto f = LookupFormByID((uint32_t)(double)args[1]);
              if (f)
                return CreateObject("Form", f);
              else
                return JsValue::Null();
            });
        } else if (s == "from") {
          if (isObjectReference) {
            f = JsValue::Function(
              [](const JsFunctionArguments& args) -> JsValue {
                CallNative::ObjectPtr from =
                  NativeValueCasts::JsObjectToNativeObject(args[1]);
                if (!from)
                  return JsValue::Null();
                auto fromRaw = (TESForm*)from->GetNativeObjectPtr();
                if (!fromRaw)
                  return JsValue::Null();
                TESObjectREFR* resRaw =
                  DYNAMIC_CAST(fromRaw, TESForm, TESObjectREFR);
                if (!resRaw)
                  return JsValue::Null();
                return CreateObject("ObjectReference", resRaw);
              });
          } else if (isActor) {
            f = JsValue::Function(
              [](const JsFunctionArguments& args) -> JsValue {
                CallNative::ObjectPtr from =
                  NativeValueCasts::JsObjectToNativeObject(args[1]);
                if (!from)
                  return JsValue::Null();
                auto fromRaw = (TESForm*)from->GetNativeObjectPtr();
                if (!fromRaw)
                  return JsValue::Null();
                Actor* resRaw = DYNAMIC_CAST(fromRaw, TESForm, Actor);
                if (!resRaw)
                  return JsValue::Null();
                return CreateObject("Actor", resRaw);
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
          f = JsValue::Function([keyStr, origin, className, callNativeArgs](
                                  const JsFunctionArguments& args) -> JsValue {
            callNativeArgs->resize(args.GetSize() + 3);
            for (size_t i = 1; i < args.GetSize(); ++i)
              (*callNativeArgs)[i + 3] = args[i];
            thread_local auto callNative = origin.GetProperty("callNative");
            return callNative.Call(*callNativeArgs);
          });
        }
      }
      return f;
    }));
  return JsValue::GlobalObject().GetProperty("Proxy").Constructor(
    { JsValue::Undefined(), skyrimPlatformExports, handler });
}
}

JsValue SkyrimPlatformProxy::Attach(const JsValue& skyrimPlatformExports)
{
  thread_local std::unordered_map<std::string, JsValue> g_classProxies;

  auto handler = JsValue::Object();
  handler.SetProperty(
    "get", ProxyGetter([=](const JsValue& origin, const JsValue& keyStr) {
      auto& proxy = g_classProxies[(std::string)keyStr];
      if (proxy.GetType() != JsValue::Type::Object)
        proxy = GetProxyForClass((std::string)keyStr, skyrimPlatformExports);
      return proxy;
    }));
  return JsValue::GlobalObject().GetProperty("Proxy").Constructor(
    { JsValue::Undefined(), skyrimPlatformExports, handler });
}
