#pragma once
#include "JsEngine.h"

inline JsValue CreatePromise(const JsValue& resolver)
{
  auto standardPromise = JsValue::GlobalObject().GetProperty("Promise");
  return standardPromise.Constructor({ standardPromise, resolver });
}
