#pragma once
#include "PropertyBinding.h"
#include <map>
#include <memory>
#include <string>

class PropertyBindingFactory
{
public:
  std::map<std::string, std::shared_ptr<PropertyBinding>>
  CreateStandardPropertyBindings();

  std::shared_ptr<PropertyBinding> CreateCustomPropertyBinding(
    const std::string& propertyName);
};
