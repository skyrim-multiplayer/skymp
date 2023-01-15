#pragma once
#include "JsFunctionArguments.h"
#include <memory>

class JsFunctionArgumentsImpl : public JsFunctionArguments
  {
  public:
    JsFunctionArgumentsImpl(void** arr_, size_t n_);

    size_t GetSize() const noexcept override;

    const JsValue& operator[](size_t i) const noexcept override;

  private:
    void** const arr;
    const size_t n;
    std::unique_ptr<JsValue> undefined;
  };