#pragma once
#include "CallNative.h" // CallNative::State
#include "JsEngine.h"
#include "TaskQueue.h"
#include <RE/BSScript/IVirtualMachine.h>
#include <functional>

namespace CallNativeApi {

struct NativeCallRequirements
{
  NativeCallRequirements()
  {
    gameThrQ.reset(new TaskQueue);
    jsThrQ.reset(new TaskQueue);
  }

  RE::BSScript::IVirtualMachine* vm = nullptr;
  RE::VMStackID stackId = (RE::VMStackID)~0;

  std::shared_ptr<TaskQueue> gameThrQ, jsThrQ;
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