#include "SkyrimPlatformProxy.h"
#include "JsUtils.h"
#include "ProxyGetter.h"

namespace {

JsValue GetProxyForClass(
  const std::string& className, const JsValue& skyrimPlatformExports,
  const std::function<CallNativeApi::NativeCallRequirements()>&
    getNativeCallRequirements)
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
          f = JsValue::Function(
            [keyStr, className, callNativeArgs, getNativeCallRequirements](
              const JsFunctionArguments& args) -> JsValue {
              callNativeArgs->resize(args.GetSize() + 3);
              for (size_t i = 1; i < args.GetSize(); ++i)
                (*callNativeArgs)[i + 3] = args[i];
              return CallNativeApi::CallNative(
                JsFunctionArgumentsVectorImpl(*callNativeArgs),
                getNativeCallRequirements);
            });
        }
      }
      return f;
    }));
  return JsValue::GlobalObject().GetProperty("Proxy").Constructor(
    { JsValue::Undefined(), skyrimPlatformExports, handler });
}
}

JsValue SkyrimPlatformProxy::Attach(
  const JsValue& skyrimPlatformExports,
  const std::function<CallNativeApi::NativeCallRequirements()>&
    getNativeCallRequirements)
{
  thread_local std::unordered_map<std::string, JsValue> g_classProxies;

  auto handler = JsValue::Object();
  handler.SetProperty(
    "get", ProxyGetter([=](const JsValue& origin, const JsValue& keyStr) {
      auto& proxy = g_classProxies[(std::string)keyStr];
      if (proxy.GetType() != JsValue::Type::Object)
        proxy = GetProxyForClass((std::string)keyStr, skyrimPlatformExports,
                                 getNativeCallRequirements);
      return proxy;
    }));
  return JsValue::GlobalObject().GetProperty("Proxy").Constructor(
    { JsValue::Undefined(), skyrimPlatformExports, handler });
}
