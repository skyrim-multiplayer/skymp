#pragma once
#include "CallNativeApi.h"

#include "NapiHelper.h"

class SkyrimPlatformProxy
{
public:
  static Napi::Object Attach(
    const Napi::Object& exports,
    const std::function<CallNativeApi::NativeCallRequirements()>&
      getNativeCallRequirements);
};
