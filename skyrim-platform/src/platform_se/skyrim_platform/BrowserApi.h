#pragma once
#include "Settings.h"

#include "NapiHelper.h"

namespace BrowserApi {
enum class Backend
{
  kOff = 0,
  kTilted = 1,
  kNirnLab = 2,
};
Backend GetBackend();
bool IsVisible();
void Register(Napi::Env env, Napi::Object& exports);
}
