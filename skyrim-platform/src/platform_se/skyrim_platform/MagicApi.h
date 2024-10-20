#pragma once

#include "NapiHelper.h"

namespace MagicApi {

Napi::Value CastSpellImmediate(const Napi::CallbackInfo& info);
Napi::Value InterruptCast(const Napi::CallbackInfo& info);

Napi::Value GetAnimationVariablesFromActor(const Napi::CallbackInfo& info);
Napi::Value ApplyAnimationVariablesToActor(const Napi::CallbackInfo& info);

void Register(Napi::Env env, Napi::Object& exports);

}
