#pragma once

#include "PropertyBinding.h"

class ScriptVariablesBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "scriptVariables"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
