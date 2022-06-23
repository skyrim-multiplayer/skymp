#pragma once

namespace EventsApi {
JsValue On(const JsFunctionArguments& args);
JsValue Once(const JsFunctionArguments& args);
JsValue SendIpcMessage(const JsFunctionArguments& args);
JsValue Unsubscribe(const JsFunctionArguments& args);

void SendEvent(const char* eventName, const std::vector<JsValue>& arguments);
void Clear();

// Exceptions will be pushed to g_taskQueue
void SendAnimationEventEnter(uint32_t selfId,
                             std::string& animEventName) noexcept;
void SendAnimationEventLeave(bool animationSucceeded) noexcept;
void SendPapyrusEventEnter(uint32_t selfId,
                           std::string& papyrusEventName) noexcept;
void SendPapyrusEventLeave() noexcept;

JsValue GetHooks();

inline void Register(JsValue& exports)
{
  exports.SetProperty("on", JsValue::Function(On));
  exports.SetProperty("once", JsValue::Function(Once));
  exports.SetProperty("hooks", GetHooks());
  exports.SetProperty("sendIpcMessage", JsValue::Function(SendIpcMessage));
  exports.SetProperty("unsubscribe", JsValue::Function(Unsubscribe));
}
}
