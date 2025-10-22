#pragma once
#include "Settings.h"

#include "NapiHelper.h"

namespace BrowserApi {
enum class Backend
{
  kTilted = 1,
  kNirnlab = 2,
};
Backend GetBackend();
void Register(Napi::Env env, Napi::Object& exports);
}
