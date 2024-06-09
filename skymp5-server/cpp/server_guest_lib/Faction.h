#pragma once
#include <nlohmann/json.hpp>
#include <simdjson.h>

struct Faction
{
  uint32_t formId;
  int8_t rank;

  friend bool operator==(const Faction& r, const Faction& l)
  {
    return r.formId == l.formId;
  }

  friend bool operator!=(const Faction& r, const Faction& l)
  {
    return !(r == l);
  }
};
