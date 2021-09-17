#pragma once
#include "JsEngine.h"

class HttpClient;

namespace HttpClientApi {
JsValue Constructor(const JsFunctionArguments& args);
JsValue Get(const JsFunctionArguments& args);
JsValue Post(const JsFunctionArguments& args);

HttpClient& GetHttpClient();

inline void Register(JsValue exports)
{
  auto httpClient = JsValue::Function(Constructor);
  exports.SetProperty("HttpClient", httpClient);
}
}
