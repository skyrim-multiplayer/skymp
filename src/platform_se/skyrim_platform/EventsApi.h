#pragma once
#include "JsEngine.h"
#include <RE/TESObjectREFR.h>

class SKSETaskInterface;

namespace EventsApi {
JsValue On(const JsFunctionArguments& args);

void SendEvent(const char* eventName, const std::vector<JsValue>& arguments);
void Clear();

// Exceptions will be pushed to g_taskQueue
void SendAnimationEventEnter(uint32_t selfId,
                             std::string& animEventName) noexcept;
void SendAnimationEventLeave(bool animationSucceeded) noexcept;

JsValue GetHooks();

inline void Register(JsValue& exports)
{
  exports.SetProperty("on", JsValue::Function(On));
  exports.SetProperty("hooks", GetHooks());
}
}