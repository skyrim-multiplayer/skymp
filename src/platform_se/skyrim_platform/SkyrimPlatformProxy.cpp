#include "SkyrimPlatformProxy.h"
#include "ProxyGetter.h"
#include <unordered_map>

namespace {
JsValue GetProxyForClass(const std::string& className,
                         const JsValue& skyrimPlatformExports)
{
  std::shared_ptr<std::unordered_map<std::string, JsValue>> functionsCache(
    new std::unordered_map<std::string, JsValue>);

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

        if (s == "from") {
          f = JsValue::Function([keyStr, origin, className, callNativeArgs,
                                 dynamicCastArgs](
                                  const JsFunctionArguments& args) -> JsValue {
            auto& from = args[1];
            (*dynamicCastArgs)[1] = from;
            thread_local auto dynamicCast = origin.GetProperty("dynamicCast");
            return dynamicCast.Call(*dynamicCastArgs);
          });
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