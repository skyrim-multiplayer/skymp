#pragma once

#include "NapiHelper.h"

namespace InventoryApi {

Napi::Value GetExtraContainerChanges(const Napi::CallbackInfo &info);
Napi::Value GetContainer(const Napi::CallbackInfo &info);
Napi::Value SetInventory(const Napi::CallbackInfo &info);
Napi::Value CastSpellImmediate(const Napi::CallbackInfo &info);

void Register(Napi::Env env, Napi::Value& exports);
}
