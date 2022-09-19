#pragma once

namespace NetworkingClientApi {
#ifdef _SP_WITH_NETWORKING_CLIENT
JsValue Create(const JsFunctionArguments& args);
JsValue Destroy(const JsFunctionArguments& args);
JsValue IsConnected(const JsFunctionArguments& args);
JsValue Tick(const JsFunctionArguments& args);
JsValue Send(const JsFunctionArguments& args);
#endif

inline void Register(JsValue& exports)
{
#ifdef _SP_WITH_NETWORKING_CLIENT
  auto networkingClient = JsValue::Object();
  networkingClient.SetProperty("create", JsValue::Function(Create));
  networkingClient.SetProperty("destroy", JsValue::Function(Destroy));
  networkingClient.SetProperty("isConnected", JsValue::Function(IsConnected));
  networkingClient.SetProperty("tick", JsValue::Function(Tick));
  networkingClient.SetProperty("send", JsValue::Function(Send));
  exports.SetProperty("networkingClient", networkingClient);
#endif
}
}
