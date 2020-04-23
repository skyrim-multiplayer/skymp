#pragma once
#include "JsEngine.h"

namespace ConsoleApi {
JsValue Log(const JsFunctionArguments& args);

inline void Register(JsValue &exports)
{
  exports.SetProperty("log", JsValue::Function(Log));
}
}