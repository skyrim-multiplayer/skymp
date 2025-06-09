#include "CallNativeApi.h"
#include "CallNative.h"
#include "CreatePromise.h"
#include "NullPointerException.h"
#include "Override.h"
#include "SP3NativeValueCasts.h"
#include "VmProvider.h"

#include <RE/T/TESDataHandler.h>

#include <spdlog/spdlog.h>

namespace {

template <class CallbackInfoLike>
Napi::Value CallNativeImpl(
  const CallbackInfoLike& info,
  const std::function<CallNativeApi::NativeCallRequirements()>&
    getNativeCallRequirements)
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
#ifndef ENABLE_SKYRIM_VR
      auto numFiles = dataHandler->compiledFileCollection.files.size();
#else
      auto numFiles = dataHandler->VRcompiledFileCollection->files.size();
#endif
      return Napi::Number::New(info.Env(), numFiles);

    } else if (!stricmp("GetModName", functionName.data())) {

      constexpr int kLightModOffset = 0x100;

      int index = NapiHelper::ExtractInt32(info[nativeArgsStart], "index");

      auto dataHandler = RE::TESDataHandler::GetSingleton();
      if (!dataHandler) {
        throw NullPointerException("dataHandler");
      }

#ifndef ENABLE_SKYRIM_VR
      auto files = dataHandler->compiledFileCollection.files;
      auto smallFiles = dataHandler->compiledFileCollection.smallFiles;
#else
      auto files = dataHandler->VRcompiledFileCollection->files;
      auto smallFiles = dataHandler->VRcompiledFileCollection->smallFiles;
#endif

      if (index > 0xff) {
        uint32_t adjusted = index - kLightModOffset;
        if (adjusted >= smallFiles.size()) {
          return Napi::String::New(info.Env(), "");
        }
        std::string s = smallFiles[adjusted]->fileName;
        return Napi::String::New(info.Env(), s);
      } else {
        if (index >= files.size()) {
          return Napi::String::New(info.Env(), "");
        }
        std::string s = files[index]->fileName;
        return Napi::String::New(info.Env(), s);
      }
    }
  }

  auto requirements = getNativeCallRequirements();
  if (!requirements.vm)
    throw std::runtime_error('\'' + className + '.' + functionName +
                             "' can't be called in this context");

  CallNative::AnySafe nativeArgs[CallNative::g_maxArgs + 1];
  auto n =
    (size_t)std::max(static_cast<int>(info.Length()) - nativeArgsStart, 0);

  for (size_t i = 0; i < n; ++i) {
    nativeArgs[i] = SP3NativeValueCasts::GetSingleton().JsValueToNativeValue(
      info[nativeArgsStart + i]);
  }

  if (!requirements.gameThrQ) {
    throw NullPointerException("gameThrQ");
  }

  if (!requirements.jsThrQ) {
    throw NullPointerException("jsThrQ");
  }

  auto& provider = VmProvider::GetSingleton();

  CallNative::Arguments callNativeArgs{
    requirements.vm,
    requirements.stackId,
    className,
    functionName,
    SP3NativeValueCasts::GetSingleton().JsObjectToNativeObject(self),
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

    auto g_promiseFn =
      Napi::Function::New(info.Env(), [](const Napi::CallbackInfo& info) {
        std::shared_ptr<Napi::Reference<Napi::Function>> resolveFunctionRef(
          new Napi::Reference<Napi::Function>(
            Napi::Persistent<Napi::Function>(info[0].As<Napi::Function>())));

        if (!g_callNativeArgsPtr)
          throw NullPointerException("g_callNativeArgsPtr");
        g_callNativeArgsPtr->latentCallback =
          [resolveFunctionRef](Napi::Env env, const CallNative::AnySafe& v) {
            spdlog::info("Latent callback called");
            resolveFunctionRef->Value().Call(
              env.Undefined(),
              { SP3NativeValueCasts::GetSingleton().NativeValueToJsValue(env,
                                                                         v) });
          };
        CallNative::CallNativeSafe(*g_callNativeArgsPtr);
        return info.Env().Undefined();
      });
    return CreatePromise(g_promiseFn);
  } else {
    Override o;
    auto res = SP3NativeValueCasts::GetSingleton().NativeValueToJsValue(
      info.Env(), CallNative::CallNativeSafe(callNativeArgs));
    return res;
  }
}

class PseudoCallbackInfo
{
public:
  PseudoCallbackInfo(Napi::Env env_, const std::vector<Napi::Value>& args_)
    : env(env_)
    , args(args_)
  {
  }

  size_t Length() const { return args.size(); }

  Napi::Value operator[](size_t i) const
  {
    if (i >= args.size()) {
      return env.Undefined();
    }
    return args[i];
  }

  Napi::Env Env() const { return env; }

private:
  const std::vector<Napi::Value>& args;
  const Napi::Env env;
};
}

Napi::Value CallNativeApi::CallNative(
  Napi::Env env, const std::vector<Napi::Value>& args,
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
  auto form =
    SP3NativeValueCasts::GetSingleton().JsValueToNativeValue(info[0]);
  auto targetType = NapiHelper::ExtractString(info[1], "targetType");
  return SP3NativeValueCasts::GetSingleton().NativeValueToJsValue(
    info.Env(), CallNative::DynamicCast(targetType, form));
}
