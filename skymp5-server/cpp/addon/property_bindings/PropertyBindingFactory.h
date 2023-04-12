#pragma once
#include <map>
#include <memory>
#include <string>
#include "PropertyBinding.h"

class PropertyBindingFactory {
public:
    std::map<std::string, std::shared_ptr<PropertyBinding>> CreateStandardPropertyBindings();

    std::shared_ptr<PropertyBinding> CreateCustomPropertyBinding(const std::string &propertyName);
};
