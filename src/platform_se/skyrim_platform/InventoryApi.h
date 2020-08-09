#pragma once
#include "JsEngine.h"

namespace InventoryApi {

JsValue GetExtraContainerChanges(const JsFunctionArguments& args);
JsValue GetContainer(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  exports.SetProperty("getExtraContainerChanges",
                      JsValue::Function(GetExtraContainerChanges));
  exports.SetProperty("getContainer", JsValue::Function(GetContainer));
}
}