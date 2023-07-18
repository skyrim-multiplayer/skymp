#pragma once
#include "PropertyBinding.h"

class IdxBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "idx"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
