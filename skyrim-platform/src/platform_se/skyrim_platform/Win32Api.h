#pragma once
#include "JsEngine.h"

namespace Win32Api {
JsValue LoadUrl(const JsFunctionArguments& args);

JsValue ExitProcess(const JsFunctionArguments& args);

JsValue FileInfo(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  auto win32 = JsValue::Object();
  win32.SetProperty(
    "loadUrl",
    JsValue::Function([](const JsFunctionArguments& args) -> JsValue {
      return LoadUrl(args);
    }));

  win32.SetProperty(
    "exitProcess",
    JsValue::Function([](const JsFunctionArguments& args) -> JsValue {
      return ExitProcess(args);
    }));

  // XXX: it's probably a bad place for it...
  win32.SetProperty(
    "fileInfo",
    JsValue::Function([](const JsFunctionArguments& args) -> JsValue {
      return FileInfo(args);
    }));

  exports.SetProperty("win32", win32);
}
}
