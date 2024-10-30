#include "SkyrimPlatformProxy.h"
#include "JsUtils.h"
#include "NullPointerException.h"
#include "ProxyGetter.h"

namespace {

Napi::Object GetProxyForClass(
  const std::string& className, const Napi::Object& skyrimPlatformExports,
  const std::function<CallNativeApi::NativeCallRequirements()>&
    getNativeCallRequirements)
{
  auto env = skyrimPlatformExports.Env();

  std::shared_ptr<std::unordered_map<
    std::string, std::shared_ptr<Napi::Reference<Napi::Function>>>>
    functionsCache(
      new std::unordered_map<
        std::string, std::shared_ptr<Napi::Reference<Napi::Function>>>);

  bool isGame = !stricmp(className.data(), "Game");
  bool isObjectReference = !stricmp(className.data(), "ObjectReference");
  bool isActor = !stricmp(className.data(), "Actor");

  auto handler = Napi::Object::New(env);
  handler.Set(
    "get",
    ProxyGetter(
      env,
      [=](const Napi::Object& origin, const std::string& keyStrExtracted) {
        auto env = origin.Env();

        std::shared_ptr<Napi::Reference<Napi::Function>>& fRef =
          (*functionsCache)[keyStrExtracted];

        if (!fRef) {
          std::shared_ptr<Napi::Reference<Napi::Object>> originRef;
          originRef.reset(
            new Napi::Reference<Napi::Object>(Napi::Persistent(origin)));

          if (isGame && keyStrExtracted == "getFormEx") {
            auto f = Napi::Function::New(
              env,
              NapiHelper::WrapCppExceptions(
                [](const Napi::CallbackInfo& info) -> Napi::Value {
                  auto f = RE::TESForm::LookupByID(
                    NapiHelper::ExtractUInt32(info[0], "formId"));
                  if (f)
                    return CreateObject(info.Env(), "Form", f);
                  else
                    return info.Env().Null();
                }));
            fRef.reset(
              new Napi::Reference<Napi::Function>(Napi::Persistent(f)));
          } else if (keyStrExtracted == "from") {
            if (isObjectReference) {
              auto f = Napi::Function::New(
                env,
                NapiHelper::WrapCppExceptions(
                  [](const Napi::CallbackInfo& info) -> Napi::Value {
                    CallNative::ObjectPtr obj =
                      NativeValueCasts::JsObjectToNativeObject(
                        NapiHelper::ExtractObject(info[0], "obj"));
                    if (!obj)
                      return info.Env().Null();
                    auto form = (RE::TESForm*)obj->GetNativeObjectPtr();
                    if (!form)
                      return info.Env().Null();
                    auto objRefr = form->As<RE::TESObjectREFR>();
                    if (!objRefr)
                      return info.Env().Null();
                    return CreateObject(info.Env(), "ObjectReference",
                                        objRefr);
                  }));
              fRef.reset(
                new Napi::Reference<Napi::Function>(Napi::Persistent(f)));
            } else if (isActor) {
              auto f = Napi::Function::New(
                env,
                NapiHelper::WrapCppExceptions(
                  [](const Napi::CallbackInfo& info) -> Napi::Value {
                    CallNative::ObjectPtr obj =
                      NativeValueCasts::JsObjectToNativeObject(
                        NapiHelper::ExtractObject(info[0],
                                                  "objActorCandidate"));
                    if (!obj)
                      return info.Env().Null();
                    auto form = (RE::TESForm*)obj->GetNativeObjectPtr();
                    if (!form)
                      return info.Env().Null();
                    auto actor = form->As<RE::Actor>();
                    if (!actor)
                      return info.Env().Null();
                    return CreateObject(info.Env(), "Actor", actor);
                  }));
              fRef.reset(
                new Napi::Reference<Napi::Function>(Napi::Persistent(f)));
            } else {
              auto f = Napi::Function::New(
                env,
                NapiHelper::WrapCppExceptions(
                  [originRef,
                   className](const Napi::CallbackInfo& info) -> Napi::Value {
                    Napi::Value from = info[0];
                    Napi::Value to = Napi::String::New(info.Env(), className);
                    auto dynamicCast = NapiHelper::ExtractFunction(
                      originRef->Value().Get("dynamicCast"), "dynamicCast");
                    return dynamicCast.Call(originRef->Value(), { from, to });
                  }));
              fRef.reset(
                new Napi::Reference<Napi::Function>(Napi::Persistent(f)));
            }
          } else {
            auto f = Napi::Function::New(
              env,
              NapiHelper::WrapCppExceptions([keyStrExtracted, className,
                                             getNativeCallRequirements](
                                              const Napi::CallbackInfo& info)
                                              -> Napi::Value {
                auto keyStr = Napi::String::New(info.Env(), keyStrExtracted);
                auto jClassName = Napi::String::New(info.Env(), className);

                std::vector<Napi::Value> callNativeArgs{ jClassName, keyStr };

                for (size_t i = 0; i < info.Length(); i++) {
                  callNativeArgs.push_back(info[i]);
                }

                return CallNativeApi::CallNative(info.Env(), callNativeArgs,
                                                 getNativeCallRequirements);
              }));
            fRef.reset(
              new Napi::Reference<Napi::Function>(Napi::Persistent(f)));
          }
        }
        if (!fRef) {
          throw NullPointerException("fRef");
        }
        return fRef->Value();
      }));

  Napi::Function proxyConstructor =
    NapiHelper::ExtractFunction(env.Global().Get("Proxy"), "Proxy");
  Napi::Object proxyObject = NapiHelper::ExtractObject(
    proxyConstructor.New({ skyrimPlatformExports, handler }), "proxyObject");
  return proxyObject;
}
}

Napi::Object SkyrimPlatformProxy::Attach(
  Napi::Object skyrimPlatformExports,
  const std::function<CallNativeApi::NativeCallRequirements()>&
    getNativeCallRequirements)
{
  auto env = skyrimPlatformExports.Env();

  thread_local std::unordered_map<
    std::string, std::shared_ptr<Napi::Reference<Napi::Object>>>
    g_classProxies;

  std::shared_ptr<Napi::Reference<Napi::Object>> skyrimPlatformExportsRef;
  skyrimPlatformExportsRef.reset(new Napi::Reference<Napi::Object>(
    Napi::Persistent(skyrimPlatformExports)));

  auto handler = Napi::Object::New(env);
  handler.Set(
    "get",
    ProxyGetter(env,
                [getNativeCallRequirements, skyrimPlatformExportsRef](
                  const Napi::Object& origin,
                  const std::string& keyStrExtracted) -> Napi::Value {
                  auto& proxyRef = g_classProxies[keyStrExtracted];
                  if (!proxyRef) {
                    auto proxy = GetProxyForClass(
                      keyStrExtracted, skyrimPlatformExportsRef->Value(),
                      getNativeCallRequirements);
                    proxyRef.reset(new Napi::Reference<Napi::Object>(
                      Napi::Persistent(proxy)));
                  }
                  return proxyRef->Value();
                }));

  Napi::Function proxyConstructor =
    NapiHelper::ExtractFunction(env.Global().Get("Proxy"), "Proxy");
  Napi::Object proxyObject = proxyConstructor.New({ skyrimPlatformExports, handler });
  return proxyObject;
}
