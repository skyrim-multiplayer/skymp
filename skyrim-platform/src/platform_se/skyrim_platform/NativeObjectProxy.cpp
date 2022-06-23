#include "NativeObjectProxy.h"
#include "CallNative.h"
#include "CallNativeApi.h"
#include "ProxyGetter.h"

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

namespace {
struct PerClassCache
{
  std::unordered_map<std::string, JsValue> funcsCache;
  JsValue prototype;
};
}

void NativeObjectProxy::Attach(JsValue& obj, const std::string& cacheClassName,
                               const JsValue& toString, const JsValue& toJson)
{

  thread_local std::unordered_map<std::string, std::shared_ptr<PerClassCache>>
    g_classCache;

  auto& classCache = g_classCache[cacheClassName];
  if (!classCache)
    classCache.reset(new PerClassCache);
  if (classCache->prototype.GetType() != JsValue::Type::Object) {

    auto handler = JsValue::Object();
    auto proxyTarget = JsValue::Object();
    proxyTarget.SetProperty("toString", toString);
    proxyTarget.SetProperty("toJSON", toJson);

    handler.SetProperty(
      "get",
      ProxyGetter([classCache, cacheClassName](const JsValue& origin,
                                               const JsValue& keyStr) {
        auto& f = classCache->funcsCache[(std::string)keyStr];
        if (f.GetType() != JsValue::Type::Function) {

          thread_local JsValue callNativeApi = [] {
            auto e = JsValue::Object();
            CallNativeApi::Register(e,
                                    [] { return g_nativeCallRequirements; });
            return e;
          }();

          std::shared_ptr<std::vector<JsValue>> callNativeArgs(
            new std::vector<JsValue>{ callNativeApi, cacheClassName, keyStr });

          f = JsValue::Function([cacheClassName, keyStr, callNativeArgs](
                                  const JsFunctionArguments& args) -> JsValue {
            callNativeArgs->resize(args.GetSize() + 3);
            (*callNativeArgs)[3] = args[0]; // self
            for (size_t i = 1; i < args.GetSize(); ++i)
              (*callNativeArgs)[i + 3] = args[i];
            thread_local auto callNative =
              callNativeApi.GetProperty("callNative");
            return callNative.Call(*callNativeArgs);
          });
        }
        return f;
      }));

    classCache->prototype =
      JsValue::GlobalObject().GetProperty("Proxy").Constructor(
        { JsValue::Undefined(), proxyTarget, handler });
  }

  auto standardObjectApi = JsValue::GlobalObject().GetProperty("Object");
  standardObjectApi.GetProperty("setPrototypeOf")
    .Call({ standardObjectApi, obj, classCache->prototype });
}
