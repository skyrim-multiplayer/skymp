#pragma once
#include "JsEngine.h"

#include "../platform_se/skyrim_platform/NapiHelper.h"

class HttpClient;

namespace HttpClientApi {
Napi::Value Constructor(const Napi::CallbackInfo& info);
Napi::Value Get(const Napi::CallbackInfo& info);
Napi::Value Post(const Napi::CallbackInfo& info);

HttpClient& GetHttpClient();

inline void Register(Napi::Env env, Napi::Object exports)
{
  auto httpClient = Napi::Function::New(env, NapiHelper::WrapCppExceptions(Constructor));
  exports.Set("HttpClient", httpClient);
}
}
