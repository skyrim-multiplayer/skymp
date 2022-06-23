#pragma once

namespace MpClientPluginApi {
JsValue GetVersion(const JsFunctionArguments& args);
JsValue CreateClient(const JsFunctionArguments& args);
JsValue DestroyClient(const JsFunctionArguments& args);
JsValue IsConnected(const JsFunctionArguments& args);
JsValue Tick(const JsFunctionArguments& args);
JsValue Send(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  auto mpClientPlugin = JsValue::Object();
  mpClientPlugin.SetProperty("getVersion", JsValue::Function(GetVersion));
  mpClientPlugin.SetProperty("createClient", JsValue::Function(CreateClient));
  mpClientPlugin.SetProperty("destroyClient",
                             JsValue::Function(DestroyClient));
  mpClientPlugin.SetProperty("isConnected", JsValue::Function(IsConnected));
  mpClientPlugin.SetProperty("tick", JsValue::Function(Tick));
  mpClientPlugin.SetProperty("send", JsValue::Function(Send));
  exports.SetProperty("mpClientPlugin", mpClientPlugin);
}
}
