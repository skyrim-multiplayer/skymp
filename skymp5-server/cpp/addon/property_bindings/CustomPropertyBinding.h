#pragma once
#include "PropertyBinding.h"

class CustomPropertyBinding : public PropertyBinding
{
public:
  explicit CustomPropertyBinding(const std::string& propertyName);

  std::string GetPropertyName() const override;
  Napi::Value Get(Napi::Env env, ScampServer& scampServer,
                  uint32_t formId) override;
  void Set(Napi::Env env, ScampServer& scampServer, uint32_t formId,
           Napi::Value newValue) override;

private:
  std::string propertyName;
  bool isPrivate = false;
};
