#pragma once
#include "Settings.h"
#include "TPOverlayService.h"

namespace BrowserApi {
struct State
{
  std::shared_ptr<OverlayService> overlayService;
};

JsValue SetVisible(const JsFunctionArguments& args);
JsValue IsVisible(const JsFunctionArguments& args);
JsValue SetFocused(const JsFunctionArguments& args);
JsValue IsFocused(const JsFunctionArguments& args);
JsValue LoadUrl(const JsFunctionArguments& args, std::shared_ptr<State> state);
JsValue GetToken(const JsFunctionArguments& args);
JsValue ExecuteJavaScript(const JsFunctionArguments& args,
                          std::shared_ptr<State> state);

inline void Register(JsValue& exports, std::shared_ptr<State> state)
{
  auto browser = JsValue::Object();
  browser.SetProperty("setVisible", JsValue::Function(SetVisible));
  browser.SetProperty("isVisible", JsValue::Function(IsVisible));
  browser.SetProperty("setFocused", JsValue::Function(SetFocused));
  browser.SetProperty("isFocused", JsValue::Function(IsFocused));
  browser.SetProperty(
    "loadUrl",
    JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
      return LoadUrl(args, state);
    }));
  browser.SetProperty("getToken", JsValue::Function(GetToken));
  browser.SetProperty(
    "executeJavaScript",
    JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
      return ExecuteJavaScript(args, state);
    }));
  exports.SetProperty("browser", browser);
}
}
