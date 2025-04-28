#pragma once

#include "CallNativeApi.h"
#include "NapiHelper.h"
#include <functional>

namespace Sp3Api {
void Register(Napi::Env env, Napi::Object& exports,
              std::function<CallNativeApi::NativeCallRequirements()>
                getNativeCallRequirements);

Napi::Value SP3ListClasses(const Napi::CallbackInfo& info);
Napi::Value SP3GetBaseClass(const Napi::CallbackInfo& info);
Napi::Value SP3ListStaticFunctions(const Napi::CallbackInfo& info);
Napi::Value SP3ListMethods(const Napi::CallbackInfo& info);
Napi::Value SP3GetFunctionImplementation(const Napi::CallbackInfo& info);
Napi::Value SP3DynamicCast(const Napi::CallbackInfo& info);
Napi::Value SP3GetCurrentTickId(const Napi::CallbackInfo& info);
Napi::Value SP3RegisterWrapObjectFunction(const Napi::CallbackInfo& info);

std::shared_ptr<Napi::FunctionReference> GetWrapObjectFunction();

}
