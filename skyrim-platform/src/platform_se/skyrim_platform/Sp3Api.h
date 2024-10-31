#pragma once

#include "NapiHelper.h"

namespace Sp3Api {
void Register(Napi::Env env, Napi::Object& exports);

Napi::Value SP3ListClasses(const Napi::CallbackInfo& info);
Napi::Value SP3GetBaseClass(const Napi::CallbackInfo& info);
Napi::Value SP3ListStaticFunctions(const Napi::CallbackInfo& info);
Napi::Value SP3ListMethods(const Napi::CallbackInfo& info);
Napi::Value SP3GetFunctionImplementation(const Napi::CallbackInfo& info);
Napi::Value SP3DynamicCast(const Napi::CallbackInfo& info);

}
