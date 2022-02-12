#pragma once
#include "JsEngine.h"

namespace Win32Api {

JsValue LoadUrl(const JsFunctionArguments& args);

JsValue ExitProcess(const JsFunctionArguments& args);

JsValue FileInfo(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  auto win32 = JsValue::Object();
  win32.SetProperty("loadUrl", LoadUrl);
  win32.SetProperty("exitProcess", ExitProcess);
  win32.SetProperty("fileInfo", FileInfo);
  exports.SetProperty("win32", win32);
}

}
