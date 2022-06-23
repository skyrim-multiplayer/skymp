#pragma once
#include "CallNative.h" // CallNative::State

namespace CallNativeApi {

struct NativeCallRequirements
{
  NativeCallRequirements()
  {
    gameThrQ = std::make_shared<Viet::TaskQueue>();
    jsThrQ = std::make_shared<Viet::TaskQueue>();
  }

  IVM* vm = nullptr;
  StackID stackId = std::numeric_limits<StackID>::max();

  std::shared_ptr<Viet::TaskQueue> gameThrQ, jsThrQ;
};

JsValue CallNative(
  const JsFunctionArguments& args,
  const std::function<NativeCallRequirements()>& getNativeCallRequirements);

JsValue DynamicCast(
  const JsFunctionArguments& args,
  const std::function<NativeCallRequirements()>& getNativeCallRequirements);

inline void Register(
  JsValue& exports,
  std::function<NativeCallRequirements()> getNativeCallRequirements)
{
  exports.SetProperty("callNative", JsValue::Function([=](auto& args) {
                        return CallNative(args, getNativeCallRequirements);
                      }));
  exports.SetProperty("dynamicCast", JsValue::Function([=](auto& args) {
                        return DynamicCast(args, getNativeCallRequirements);
                      }));
}
}
