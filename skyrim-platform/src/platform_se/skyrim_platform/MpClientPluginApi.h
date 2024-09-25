#pragma once

#include "NapiHelper.h"

namespace MpClientPluginApi {
Napi::Value GetVersion(const Napi::CallbackInfo& info);
Napi::Value CreateClient(const Napi::CallbackInfo& info);
Napi::Value DestroyClient(const Napi::CallbackInfo& info);
Napi::Value IsConnected(const Napi::CallbackInfo& info);
Napi::Value Tick(const Napi::CallbackInfo& info);
Napi::Value Send(const Napi::CallbackInfo& info);

inline void Register(Napi::Env env, Napi::Object& exports)
{
  auto mpClientPlugin = Napi::Object::New(env);
  mpClientPlugin.Set(
    "getVersion",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetVersion)));
  mpClientPlugin.Set(
    "createClient",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(CreateClient)));
  mpClientPlugin.Set(
    "destroyClient",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(DestroyClient)));
  mpClientPlugin.Set(
    "isConnected",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(IsConnected)));
  mpClientPlugin.Set(
    "tick", Napi::Function::New(env, NapiHelper::WrapCppExceptions(Tick)));
  mpClientPlugin.Set(
    "send", Napi::Function::New(env, NapiHelper::WrapCppExceptions(Send)));
  exports.Set("mpClientPlugin", mpClientPlugin);
}
}
