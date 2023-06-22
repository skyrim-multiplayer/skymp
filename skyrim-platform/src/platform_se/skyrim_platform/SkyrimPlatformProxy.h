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

class JsFunctionArgumentsVectorImpl : public JsFunctionArguments
{
public:
  JsFunctionArgumentsVectorImpl(const std::vector<JsValue>& arr_)
    : arr(arr_)
    , n(arr_.size())
  {
  }

  size_t GetSize() const noexcept override { return n; }

  const JsValue& operator[](size_t i) const noexcept override
  {
    return arr[i];
  }

private:
  const std::vector<JsValue>& arr;
  const size_t n;
};
