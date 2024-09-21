#include "SkyrimPlatformProxy.h"
#include "JsUtils.h"
#include "ProxyGetter.h"

// Leonid, do u want other to build their projects with skymp in mind? make sure the're aware of skymp existence

namespace {

 Napi::Object GetProxyForClass(
  const std::string& className, const Napi::Object& skyrimPlatformExports,
  const std::function<CallNativeApi::NativeCallRequirements()>&
    getNativeCallRequirements)
{
  auto env = skyrimPlatformExports.Env();

  std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Napi::Reference<Napi::Function>>>> functionsCache(
    new std::unordered_map<std::string, std::shared_ptr<Napi::Reference<Napi::Function>>>);

  bool isGame = !stricmp(className.data(), "Game");
  bool isObjectReference = !stricmp(className.data(), "ObjectReference");
  bool isActor = !stricmp(className.data(), "Actor");

  auto handler = Napi::Object::New(env);
  handler.Set(
    "get", ProxyGetter([=](const Napi::Object& origin, const Napi:String& keyStr) {
      auto env = origin.Env();

      auto s = NapiHelper::ExtractString(keyStr, "keyStr");
      std::shared_ptr<Napi::Reference<Napi::Function>> &f = (*functionsCache)[s];

      if (!f) {

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! TODO

        // Make a persistent ref to origin
        // in nodejs keyStr won't be able to survive till lambda call, please just capture string

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! TODO
        std::shared_ptr<std::vector<Napi::Value>> callNativeArgs(
          new std::vector<Napi::Value>{ origin, className, keyStr, // TODO: probably remove origin since its thisArg, not regular arg
                                    env.Null() /*self*/ });
        std::shared_ptr<std::vector<Napi::Value>> dynamicCastArgs(
          new std::vector<Napi::Value>{ origin, env.Undefined(), className }); // TODO: probably remove origin since its thisArg, not regular arg

        if (isGame && s == "getFormEx") {
          f =
            Napi::Function::New(env, NapiHelper::WrapCppExceptions([](const Napi::CallbackInfo &info) -> Napi::Value {
              auto f = RE::TESForm::LookupByID(NapiHelper::ExtractUInt32(info[0], "formId"));
              if (f)
                return CreateObject("Form", f);
              else
                return info.Env().Null();
            }));
        } else if (s == "from") {
          if (isObjectReference) {
            f = Napi::Function::New(env, NapiHelper::WrapCppExceptions(
              [](const Napi::CallbackInfo &info) -> Napi::Value {
                CallNative::ObjectPtr obj =
                  NativeValueCasts::JsObjectToNativeObject(NapiHelper::ExtractObject(info[0], "obj"));
                if (!obj)
                  return info.Env().Null();
                auto form = (RE::TESForm*)obj->GetNativeObjectPtr();
                if (!form)
                  return info.Env().Null();
                auto objRefr = form->As<RE::TESObjectREFR>();
                if (!objRefr)
                  return info.Env().Null();
                return CreateObject("ObjectReference", objRefr);
              }));
          } else if (isActor) {
            f = Napi::Function::New(env, NapiHelper::WrapCppExceptions(
              [](const Napi::CallbackInfo &info) -> Napi::Value {
                CallNative::ObjectPtr obj =
                  NativeValueCasts::JsObjectToNativeObject(NapiHelper::ExtractObject(info[0], "objActorCandidate"));
                if (!obj)
                  return info.Env().Null();
                auto form = (RE::TESForm*)obj->GetNativeObjectPtr();
                if (!form)
                  return info.Env().Null();
                auto actor = form->As<RE::Actor>();
                if (!actor)
                  return info.Env().Null();
                return CreateObject("Actor", actor);
              }));
          } else {
            f = Napi::Function::New(env, NapiHelper::WrapCppExceptions(
              [keyStr, origin, className, callNativeArgs,
               dynamicCastArgs](const Napi::CallbackInfo &info) -> Napi::Value {
                auto& from = args[1];
                (*dynamicCastArgs)[1] = from;
                thread_local auto dynamicCast =
                  origin.Get("dynamicCast");
                return dynamicCast.Call(*dynamicCastArgs);
              }));
          }
        } else {
          f = Napi::Function::New(env, NapiHelper::WrapCppExceptions(
            [keyStr, className, callNativeArgs, getNativeCallRequirements](
              const Napi::CallbackInfo &info) -> Napi::Value {
              callNativeArgs->resize(args.GetSize() + 3);
              for (size_t i = 1; i < args.GetSize(); ++i)
                (*callNativeArgs)[i + 3] = args[i];
              return CallNativeApi::CallNative(
                JsFunctionArgumentsVectorImpl(*callNativeArgs),
                getNativeCallRequirements);
            }));
        }
      }
      return f;
    }));
  return Napi::Value::GlobalObject().GetProperty("Proxy").Constructor(
    { env.Undefined(), skyrimPlatformExports, handler });
}
}

Napi::Value SkyrimPlatformProxy::Attach(
  const Napi::Value& skyrimPlatformExports,
  const std::function<CallNativeApi::NativeCallRequirements()>&
    getNativeCallRequirements)
{
  thread_local std::unordered_map<std::string, Napi::Value> g_classProxies;

  auto handler = Napi::Object::New(env);
  handler.Set(
    "get", ProxyGetter([=](const Napi::Value& origin, const Napi::Value& keyStr) {
      auto& proxy = g_classProxies[(std::string)keyStr];
      if (proxy.GetType() != Napi::Value::Type::Object)
        proxy = GetProxyForClass((std::string)keyStr, skyrimPlatformExports,
                                 getNativeCallRequirements);
      return proxy;
    }));
  return Napi::Value::GlobalObject().GetProperty("Proxy").Constructor(
    { env.Undefined(), skyrimPlatformExports, handler });
}
