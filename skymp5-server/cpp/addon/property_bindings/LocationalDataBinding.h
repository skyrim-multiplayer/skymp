#pragma once
#include "PropertyBinding.h"

class LocationalDataBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "locationalData"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
  void Set(Napi::Env env, ScampServer& scampServer, uint32_t formId,
           Napi::Value newValue) override;

protected:
  virtual void Apply(MpActor& actor, const LocationalData& locationalData);
};
