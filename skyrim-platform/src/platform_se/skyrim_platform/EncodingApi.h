#pragma once

#include "NapiHelper.h"

namespace EncodingApi {
Napi::Value EncodeUtf8(const Napi::CallbackInfo &info);
Napi::Value DecodeUtf8(const Napi::CallbackInfo &info);

inline void Register(Napi::Env env, Napi::Object& exports)
{
  exports.Set("encodeUtf8", Napi::Function::New(env, NapiHelper::WrapCppExceptions(EncodeUtf8)));
  exports.Set("decodeUtf8", Napi::Function::New(env, NapiHelper::WrapCppExceptions(DecodeUtf8)));
}
}
