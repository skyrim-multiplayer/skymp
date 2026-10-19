#pragma once
#include "PropertyBinding.h"

class ActorValuesBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "actorValues"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
  void Set(Napi::Env env, ScampServer& scampServer, uint32_t formId,
           Napi::Value newValue) override;
};
