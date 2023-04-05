#include "CallNativeApi.h"
#include "CallNative.h"
#include "CreatePromise.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include "Override.h"
#include "VmProvider.h"

JsValue CallNativeApi::CallNative(
  const JsFunctionArguments& args,
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
  auto n = (size_t)std::max((int)args.GetSize() - nativeArgsStart, 0);

  for (size_t i = 0; i < n; ++i)
    nativeArgs[i] =
      NativeValueCasts::JsValueToNativeValue(args[nativeArgsStart + i]);

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

JsValue CallNativeApi::DynamicCast(
  const JsFunctionArguments& args,
  const std::function<NativeCallRequirements()>& getNativeCallRequirements)
{
  auto form = NativeValueCasts::JsValueToNativeValue(args[1]);
  auto targetType = std::string(args[2]);
  return NativeValueCasts::NativeValueToJsValue(
    CallNative::DynamicCast(targetType, form));
}
