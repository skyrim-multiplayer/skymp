#pragma once

namespace NetworkingClientApi {
#ifdef _SP_WITH_NETWORKING_CLIENT
JsValue Create(const JsFunctionArguments& args);
JsValue Destroy(const JsFunctionArguments& args);
JsValue IsConnected(const JsFunctionArguments& args);
JsValue HandlePackets(const JsFunctionArguments& args);
JsValue Send(const JsFunctionArguments& args);
#endif

inline void Register(JsValue& exports)
{
#ifdef _SP_WITH_NETWORKING_CLIENT
  auto mpClientPlugin = JsValue::Object();
  mpClientPlugin.SetProperty("create", JsValue::Function(Create));
  mpClientPlugin.SetProperty("destroy",
                             JsValue::Function(Destroy));
  mpClientPlugin.SetProperty("isConnected", JsValue::Function(IsConnected));
  mpClientPlugin.SetProperty("handlePackets", JsValue::Function(HandlePackets));
  mpClientPlugin.SetProperty("send", JsValue::Function(Send));
  exports.SetProperty("mpClientPlugin", mpClientPlugin);
#endif
}
}
