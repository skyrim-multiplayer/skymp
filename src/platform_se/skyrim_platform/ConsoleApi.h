#pragma once
#include "JsEngine.h"

namespace ConsoleApi {
JsValue PrintConsole(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  exports.SetProperty("printConsole", JsValue::Function(PrintConsole));
}
}