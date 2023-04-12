#pragma once
#include "PropertyBinding.h"

class OnlinePlayersBinding : public PropertyBinding
{
public:
  std::string GetPropertyName() const override { return "onlinePlayers"; }
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
};
