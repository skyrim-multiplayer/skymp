#pragma once
#include "PropertyBinding.h"

class IsOpenBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "isOpen"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
  void Set(Napi::Env env, ScampServer& scampServer, uint32_t formId,
           Napi::Value newValue) override;
};
