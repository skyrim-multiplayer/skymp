#pragma once
#include "PropertyBinding.h"

class ActorNeighborsBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "actorNeighbors"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
