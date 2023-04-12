#pragma once
#include "PropertyBinding.h"

class IsOnlineBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "isOnline"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
