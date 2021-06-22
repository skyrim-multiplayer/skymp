#pragma once
#include "JsEngine.h"

namespace HttpClientApi {
JsValue Constructor(const JsFunctionArguments& args);
JsValue Get(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  auto httpClient = JsValue::Function(Constructor);
  exports.SetProperty("HttpClient", httpClient);
}
}