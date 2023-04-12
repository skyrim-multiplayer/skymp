#pragma once
#include "PropertyBinding.h"

class BaseDescBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "baseDesc"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
