#pragma once

#include "PropertyBinding.h"

class TemplateChainBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "templateChain"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
