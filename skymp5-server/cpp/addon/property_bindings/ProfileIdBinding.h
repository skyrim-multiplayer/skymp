#pragma once
#include "PropertyBinding.h"

class ProfileIdBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "profileId"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
