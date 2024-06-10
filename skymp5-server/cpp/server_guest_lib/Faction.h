#pragma once
#include <nlohmann/json.hpp>
#include <simdjson.h>

struct Faction
{
  uint32_t formId = 0;
  int8_t rank = 0;
};
