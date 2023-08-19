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
    undefined = std::make_unique<JsValue>(JsValue::Undefined());
  }

  size_t GetSize() const noexcept override { return n; }

  const JsValue& operator[](size_t i) const noexcept override
  {
    return i < n ? arr[i] : *undefined;
  }

private:
  const std::vector<JsValue>& arr;
  const size_t n;
  std::unique_ptr<JsValue> undefined;
};
