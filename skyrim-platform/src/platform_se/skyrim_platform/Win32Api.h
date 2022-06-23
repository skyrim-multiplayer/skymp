#pragma once

namespace Win32Api {
JsValue LoadUrl(const JsFunctionArguments& args);

JsValue ExitProcess(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  auto win32 = JsValue::Object();
  win32.SetProperty(
    "loadUrl",
    JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
      return LoadUrl(args);
    }));

  win32.SetProperty(
    "exitProcess",
    JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
      return ExitProcess(args);
    }));

  exports.SetProperty("win32", win32);
}
}
