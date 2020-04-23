#pragma once
#include "JsEngine.h"

class SKSETaskInterface;

namespace EventsApi {
JsValue On(const JsFunctionArguments& args);

void SendEvent(const char* eventName, const std::vector<JsValue>& arguments);
void Clear();

inline void Register(JsValue& exports)
{
  exports.SetProperty("on", JsValue::Function(On));
}
}