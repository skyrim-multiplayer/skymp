#pragma once
#include "ScampServer.h"
#include <cstdint>
#include <napi.h>
#include <string>

class PropertyBinding
{
public:
  virtual ~PropertyBinding() = default;
  virtual std::string GetPropertyName() const = 0;
  virtual Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                          uint32_t formId);
  virtual void Set(Napi::Env env, ScampServer& scampServer, uint32_t formId,
                   Napi::Value newValue);
};
