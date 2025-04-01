#pragma once

#include "NapiHelper.h"

namespace EventsApi {
Napi::Value On(const Napi::CallbackInfo& info);
Napi::Value Once(const Napi::CallbackInfo& info);
Napi::Value SendIpcMessage(const Napi::CallbackInfo& info);
Napi::Value Unsubscribe(const Napi::CallbackInfo& info);

void SendEvent(const char* eventName,
               const std::vector<Napi::Value>& arguments);
void Clear();

// Exceptions will be pushed to g_taskQueue
void SendAnimationEventEnter(uint32_t selfId,
                             std::string& animEventName) noexcept;
void SendAnimationEventLeave(bool animationSucceeded) noexcept;
void SendPapyrusEventEnter(uint32_t selfId,
                           std::string& papyrusEventName) noexcept;
void SendPapyrusEventLeave() noexcept;

Napi::Value GetHooks(Napi::Env env);

inline void Register(Napi::Env env, Napi::Object& exports)
{
  exports.Set("on",
              Napi::Function::New(env, NapiHelper::WrapCppExceptions(On)));
  exports.Set("once",
              Napi::Function::New(env, NapiHelper::WrapCppExceptions(Once)));
  exports.Set("hooks", GetHooks(env));
  exports.Set(
    "sendIpcMessage",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SendIpcMessage)));
  exports.Set(
    "unsubscribe",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(Unsubscribe)));
}
}
