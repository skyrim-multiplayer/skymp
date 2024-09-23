#include "CallNativeApi.h"
#include "CallNative.h"
#include "CreatePromise.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include "Override.h"
#include "VmProvider.h"

#include <RE/T/TESDataHandler.h>

namespace 
{

template <class CallbackInfoLike>
Napi::Value CallNativeImpl(
  const CallbackInfoLike& info,
  const std::function<CallNativeApi::NativeCallRequirements()>& getNativeCallRequirements)
{
  auto className = NapiHelper::ExtractString(info[0], "className");
  auto functionName = NapiHelper::ExtractString(info[1], "functionName");
  auto self = info[2];
  constexpr int nativeArgsStart = 3;

  // https://github.com/ianpatt/skse64/blob/971babc435e2620521c8556ea8ae7b9a4910ff61/skse64/PapyrusGame.cpp#L94
  if (!stricmp("Game", className.data())) {
    if (!stricmp("GetModCount", functionName.data())) {
      auto dataHandler = RE::TESDataHandler::GetSingleton();
      if (!dataHandler) {
        throw NullPointerException("dataHandler");
      }
      auto numFiles = dataHandler->compiledFileCollection.files.size();
      return Napi::Number::New(info.Env(), numFiles);

    } else if (!stricmp("GetModName", functionName.data())) {

      constexpr int kLightModOffset = 0x100;

      int index = NapiHelper::ExtractInt32(info[nativeArgsStart], "index");

      auto dataHandler = RE::TESDataHandler::GetSingleton();
      if (!dataHandler) {
        throw NullPointerException("dataHandler");
      }

      if (index > 0xff) {
        uint32_t adjusted = index - kLightModOffset;
        if (adjusted >=
            dataHandler->compiledFileCollection.smallFiles.size()) {
          return Napi::String::New(info.Env(), "");
        }
        std::string s =
          dataHandler->compiledFileCollection.smallFiles[adjusted]->fileName;
        return Napi::String::New(info.Env(), s);
      } else {
        if (index >= dataHandler->compiledFileCollection.files.size()) {
          return Napi::String::New(info.Env(), "");
        }
        std::string s =
          dataHandler->compiledFileCollection.files[index]->fileName;
        return Napi::String::New(info.Env(), s);
      }
    }
  }

  auto requirements = getNativeCallRequirements();
  if (!requirements.vm)
    throw std::runtime_error('\'' + className + '.' + functionName +
                             "' can't be called in this context");

  CallNative::AnySafe nativeArgs[CallNative::g_maxArgs + 1];
  auto n = (size_t)std::max(static_cast<int>(info.Legnth()) - nativeArgsStart, 0);

  for (size_t i = 0; i < n; ++i) {
    nativeArgs[i] =
      NativeValueCasts::JsValueToNativeValue(info[nativeArgsStart + i]);
  }

  static VmProvider provider;

  if (!requirements.gameThrQ)
    throw NullPointerException("gameThrQ");
  if (!requirements.jsThrQ)
    throw NullPointerException("jsThrQ");
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
      Napi::Function::New(info.Env(), [](const Napi::CallbackInfo& info) {
        std::shared_ptr<Napi::Reference<Napi::Function>> resolveFunctionRef(
          new Napi::Reference<Napi::Function>(
          Napi::Persistent<Napi::Function>(info[0].As<Napi::Function>())));

        if (!g_callNativeArgsPtr)
          throw NullPointerException("g_callNativeArgsPtr");
        g_callNativeArgsPtr->latentCallback =
          [resolveFunctionRef](const CallNative::AnySafe& v) {
            resolveFunctionRef->Value().Call({ info.Env().Undefined(),  NativeValueCasts::NativeValueToJsValue(v) });
          };
        CallNative::CallNativeSafe(*g_callNativeArgsPtr);
        return info.Env().Undefined();
      });
    return CreatePromise(g_promiseFn);
  } else {
    Override o;
    auto res = NativeValueCasts::NativeValueToJsValue(
      CallNative::CallNativeSafe(callNativeArgs));
    return res;
  }
}

class PseudoCallbackInfo {
public:
  PseudoCallbackInfo(Napi::Env env_, const std::vector<Napi::Value> &args_) : env(env_), args(args_)
  {
  }

  // Length method skipped

  Napi::Value operator[](size_t i) const
  {
    if (i >= args.size()) {
      return env.Undefined();
    }
    return args[i];
  }

  Napi::Env Env() const
  {
    return env;
  }

private:
  const std::vector<Napi::Value> &args;
  const Napi::Env env;
};
}

Napi::Value CallNative(
  Napi::Env env, const std::vector<Napi::Value> &args,
  const std::function<NativeCallRequirements()>& getNativeCallRequirements)
{
  PseudoCallbackInfo pseudoCallbackInfo(env, args);
  return CallNativeImpl(pseudoCallbackInfo, getNativeCallRequirements);
}

Napi::Value CallNativeApi::CallNative(
  const Napi::CallbackInfo& info,
  const std::function<NativeCallRequirements()>& getNativeCallRequirements)
{
  return CallNativeImpl(info, getNativeCallRequirements);
}

Napi::Value CallNativeApi::DynamicCast(
  const Napi::CallbackInfo& info,
  const std::function<NativeCallRequirements()>& getNativeCallRequirements)
{
  auto form = NativeValueCasts::JsValueToNativeValue(info[0]);
  auto targetType = NapiHelper::ExtractString(info[1], "targetType");
  return NativeValueCasts::NativeValueToJsValue(info.Env(),
    CallNative::DynamicCast(targetType, form));
}
