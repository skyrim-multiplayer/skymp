#pragma once
#include "Settings.h"
#include "TPOverlayService.h"

#include "NapiHelper.h"

namespace BrowserApi {
struct State
{
  std::shared_ptr<OverlayService> overlayService;
};

Napi::Value SetVisible(const Napi::CallbackInfo& info);
Napi::Value IsVisible(const Napi::CallbackInfo& info);
Napi::Value SetFocused(const Napi::CallbackInfo& info);
Napi::Value IsFocused(const Napi::CallbackInfo& info);
Napi::Value LoadUrl(const Napi::CallbackInfo& info, std::shared_ptr<State> state);
Napi::Value GetToken(const Napi::CallbackInfo& info);
Napi::Value ExecuteJavaScript(const Napi::CallbackInfo& info,
                          std::shared_ptr<State> state);

void Register(Napi::Env env, Napi::Object& exports, std::shared_ptr<State> state);
}
