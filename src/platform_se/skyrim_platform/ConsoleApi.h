#pragma once
#include "JsEngine.h"

namespace ConsoleApi {

JsValue PrintConsole(const JsFunctionArguments& args);
JsValue FindConsoleComand(const JsFunctionArguments& args);

void Clear();

inline void Register(JsValue& exports)
{
  exports.SetProperty("printConsole", JsValue::Function(PrintConsole));
  exports.SetProperty("findConsoleCommand",
                      JsValue::Function(ConsoleApi::FindConsoleComand));
}
}