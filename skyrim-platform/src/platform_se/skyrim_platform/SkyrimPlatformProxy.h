#pragma once
#include "CallNativeApi.h"

class SkyrimPlatformProxy
{
public:
  static JsValue Attach(
    const JsValue& exports,
    const std::function<CallNativeApi::NativeCallRequirements()>&
      getNativeCallRequirements);
};
