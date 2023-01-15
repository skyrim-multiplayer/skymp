#pragma once
#include <cstddef>

class JsValue;

class JsFunctionArguments
{
public:
  virtual size_t GetSize() const noexcept = 0;

  // 'this' arg is at index 0
  virtual const JsValue& operator[](size_t i) const noexcept = 0;
};