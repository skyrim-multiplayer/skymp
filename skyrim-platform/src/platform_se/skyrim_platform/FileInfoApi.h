#pragma once

#include "NapiHelper.h"

namespace FileInfoApi {

Napi::Value FileInfo(const Napi::CallbackInfo& info);

inline void Register(Napi::Env env, Napi::Value& exports)
{
  exports.Set("getFileInfo", Napi::Function::New(env, FileInfo));
}

}
