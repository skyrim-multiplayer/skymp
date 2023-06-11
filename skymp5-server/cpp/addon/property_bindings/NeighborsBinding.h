#pragma once
#include "PropertyBinding.h"

class NeighborsBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "neighbors"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
