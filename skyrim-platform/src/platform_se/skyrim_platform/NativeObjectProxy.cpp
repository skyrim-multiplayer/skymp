#include "NativeObjectProxy.h"
#include "CallNative.h"
#include "CallNativeApi.h"
#include "ProxyGetter.h"

extern CallNativeApi::NativeCallRequirements g_nativeCallRequirements;

namespace {
struct PerClassCache
{
  std::unordered_map<std::string,
                     std::shared_ptr<Napi::Reference<Napi::Function>>>
    funcsCache;
  std::shared_ptr<Napi::Reference<Napi::Object>> prototype;
};
}

void NativeObjectProxy::Attach(Napi::External<NativeObject>& obj,
                               const std::string& cacheClassName,
                               const Napi::Value& toString,
                               const Napi::Value& toJson)
{
  auto env = obj.Env();

  thread_local std::unordered_map<std::string, std::shared_ptr<PerClassCache>>
    g_classCache;

  auto& classCache = g_classCache[cacheClassName];
  if (!classCache) {
    classCache.reset(new PerClassCache);
  }
  if (!classCache->prototype) {
    auto handler = Napi::Object::New(env);
    auto proxyTarget = Napi::Object::New(env);
    proxyTarget.Set("toString", toString);
    proxyTarget.Set("toJSON", toJson);

    handler.Set(
      "get",
      ProxyGetter(
        env,
        [classCache,
         cacheClassName](const Napi::Value& origin,
                         const std::string& keyStr) -> Napi::Value {
          auto env = origin.Env();

          auto& f = classCache->funcsCache[keyStr];
          if (!f) {
            thread_local Napi::Reference<Napi::Object> g_callNativeApi = [&] {
              auto e = Napi::Object::New(env);
              CallNativeApi::Register(env, e,
                                      [] { return g_nativeCallRequirements; });
              return Napi::Persistent(e);
            }();

            f.reset(new Napi::Reference<Napi::Function>(
              Napi::Persistent(Napi::Function::New(
                env,
                NapiHelper::WrapCppExceptions(
                  [cacheClassName,
                   keyStr](const Napi::CallbackInfo& info) -> Napi::Value {
                    std::vector<Napi::Value> callNativeParams = {
                      Napi::String::New(info.Env(), cacheClassName),
                      Napi::String::New(info.Env(), keyStr), info.This()
                    };
                    for (size_t i = 0; i < info.Length(); ++i) {
                      callNativeParams.push_back(info[i]);
                    }

                    // Access the global function callNative
                    thread_local auto callNative =
                      std::shared_ptr<Napi::Reference<Napi::Function>>(
                        new Napi::Reference<Napi::Function>(
                          Napi::Persistent(g_callNativeApi.Value()
                                             .As<Napi::Object>()
                                             .Get("callNative")
                                             .As<Napi::Function>())));

                    // Ensure callNative is a function before calling it
                    if (!callNative->Value().IsFunction()) {
                      throw std::runtime_error("callNative is not a function");
                    }

                    // Call callNative with the prepared arguments
                    return callNative->Value().Call(g_callNativeApi.Value(),
                                                    callNativeParams);
                  })))));
          }
          return f->Value();
        }));
    Napi::Object global = env.Global();
    Napi::Function proxyConstructor = global.Get("Proxy").As<Napi::Function>();
    Napi::Object proxy =
      proxyConstructor.New({ env.Undefined(), proxyTarget, handler });
    classCache->prototype =
      std::make_shared<Napi::Reference<Napi::Object>>(Napi::Persistent(proxy));
  }

  Napi::Object global = env.Global();
  Napi::Object standardObjectApi = global.Get("Object").As<Napi::Object>();
  Napi::Function setPrototypeOf =
    standardObjectApi.Get("setPrototypeOf").As<Napi::Function>();
  setPrototypeOf.Call(
    standardObjectApi,
    { standardObjectApi, obj, classCache->prototype->Value() });
}
