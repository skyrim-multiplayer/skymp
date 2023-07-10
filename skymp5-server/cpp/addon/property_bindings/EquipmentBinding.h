#pragma once
#include "PropertyBinding.h"

class EquipmentBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "equipment"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
