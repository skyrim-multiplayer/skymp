#pragma once
#include "Inventory.h"

struct Equipment
{
  uint32_t numChanges = 0;
  Inventory inv;

  nlohmann::json ToJson() const;

  static Equipment FromJson(simdjson::dom::element& element);
};
