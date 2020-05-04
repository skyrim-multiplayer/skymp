#pragma once
#include "JsEngine.h"

class SkyrimPlatformProxy
{
public:
  static JsValue Attach(const JsValue& exports);
};