#pragma once
#include "PropertyBinding.h"

class LastAnimEventBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "lastAnimEvent"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
