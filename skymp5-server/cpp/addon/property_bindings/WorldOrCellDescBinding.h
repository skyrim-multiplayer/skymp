#pragma once
#include "PropertyBinding.h"

class WorldOrCellDescBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "worldOrCellDesc"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
