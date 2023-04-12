#pragma once

#include "LocationalDataBinding.h"

class SpawnPointBinding : public LocationalDataBinding
{
public:
  std::string GetPropertyName() const override { return "spawnPoint"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;

protected:
  void Apply(MpActor& actor, const LocationalData& locationalData) override;
};
