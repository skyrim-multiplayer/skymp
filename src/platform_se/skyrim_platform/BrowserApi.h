#pragma once
#include "JsEngine.h"
#include "TPOverlayService.h"
#include <memory>

namespace BrowserApi {
struct State
{
  std::shared_ptr<OverlayService> overlayService;
};

JsValue SetVisible(const JsFunctionArguments& args);
JsValue SetFocused(const JsFunctionArguments& args);
JsValue LoadUrl(const JsFunctionArguments& args, std::shared_ptr<State> state);
JsValue GetToken(const JsFunctionArguments& args);

inline void Register(JsValue& exports, std::shared_ptr<State> state)
{
  auto browser = JsValue::Object();
  browser.SetProperty("setVisible", JsValue::Function(SetVisible));
  browser.SetProperty("setFocused", JsValue::Function(SetFocused));
  browser.SetProperty(
    "loadUrl",
    JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
      return LoadUrl(args, state);
    }));
  browser.SetProperty("getToken", JsValue::Function(GetToken));
  exports.SetProperty("browser", browser);
}
}