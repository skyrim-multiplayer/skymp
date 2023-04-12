#pragma once
#include "PropertyBinding.h"

class TypeBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "type"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
