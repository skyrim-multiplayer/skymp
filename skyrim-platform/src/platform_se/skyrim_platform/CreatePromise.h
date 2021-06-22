#pragma once
#include "JsEngine.h"

inline JsValue CreatePromise(const JsValue& resolver)
{
  thread_local auto g_standardPromise =
    JsValue::GlobalObject().GetProperty("Promise");
  return g_standardPromise.Constructor({ g_standardPromise, resolver });
}