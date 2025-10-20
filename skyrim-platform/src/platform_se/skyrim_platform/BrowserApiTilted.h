#pragma once
#include "Settings.h"

#include "NapiHelper.h"

namespace BrowserApiTilted {
Napi::Value SetVisible(const Napi::CallbackInfo& info);
Napi::Value IsVisible(const Napi::CallbackInfo& info);
Napi::Value SetFocused(const Napi::CallbackInfo& info);
Napi::Value IsFocused(const Napi::CallbackInfo& info);
Napi::Value LoadUrl(const Napi::CallbackInfo& info);
Napi::Value GetToken(const Napi::CallbackInfo& info);
Napi::Value ExecuteJavaScript(const Napi::CallbackInfo& info);
}
