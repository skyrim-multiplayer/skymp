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

inline void Register(Napi::Env env, Napi::Object& exports, std::shared_ptr<State> state)
{
  auto browser = Napi::Object::New(env);
  browser.Set("setVisible", Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetVisible)));
  browser.Set("isVisible", Napi::Function::New(env, NapiHelper::WrapCppExceptions(IsVisible)));
  browser.Set("setFocused", Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetFocused)));
  browser.Set("isFocused", Napi::Function::New(env, NapiHelper::WrapCppExceptions(IsFocused)));
  browser.Set(
    "loadUrl",
    Napi::Function::New(NapiHelper::WrapCppExceptions([=](const Napi::CallbackInfo& info) -> JsValue {
      return LoadUrl(info, state);
    })));
  browser.Set("getToken", Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetToken)));
  browser.Set(
    "executeJavaScript",
    NapiHelper::WrapCppExceptions(Napi::Function::New([=](const Napi::CallbackInfo& info) -> JsValue {
      return ExecuteJavaScript(info, state);
    })));
  exports.Set("browser", browser);
}
}
