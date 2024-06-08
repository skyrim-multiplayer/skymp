#pragma once
#include <nlohmann/json.hpp>
#include <simdjson.h>

struct Faction
{
  uint32_t formID;
  int8_t rank;

  friend bool operator==(const Faction& r, const Faction& l)
  {
    return r.formID == l.formID;
  }

  friend bool operator!=(const Faction& r, const Faction& l)
  {
    return !(r == l);
  }
};
