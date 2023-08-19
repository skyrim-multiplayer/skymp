#pragma once
#include "Property.h"
#include <set>

#pragma pack(push, 1)

namespace espm {
struct Script
{
  std::string scriptName;
  uint8_t status = 0;
  std::set<Property> properties;
};

}

#pragma pack(pop)
