#pragma once
#include <nlohmann/json.hpp>
#include <simdjson.h>

struct Faction
{
  uint32_t factionID;
  const char* editorID;
  int8_t rank;

  friend bool operator==(const Faction& r, const Faction& l)
  {
    return r.editorID == l.editorID;
  }

  friend bool operator!=(const Faction& r, const Faction& l)
  {
    return !(r == l);
  }
};
