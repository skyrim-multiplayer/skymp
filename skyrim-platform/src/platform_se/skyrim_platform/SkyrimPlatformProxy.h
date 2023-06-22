#pragma once
#include "CallNativeApi.h"

class SkyrimPlatformProxy
{
public:
  static JsValue Attach(const JsValue& exports,
    std::function<CallNativeApi::NativeCallRequirements()> getNativeCallRequirements);
};
