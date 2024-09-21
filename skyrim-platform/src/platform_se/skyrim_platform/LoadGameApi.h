#pragma once

#include "NapiHelper.h"

namespace LoadGameApi {
Napi::Value LoadGame(const Napi::CallbackInfo& info);

inline void Register(Napi::Env env, Napi::Object& exports)
{
  exports.Set("loadGame", Napi::Function::New(env, NapiHelper::WrapCppExceptions(LoadGame)));
}
}
