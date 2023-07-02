#pragma once
#include "Inventory.h"

struct Equipment
{
  uint32_t numChanges = 0;
  Inventory inv;

  uint32_t leftSpell = 0;
  uint32_t rightSpell = 0;
  uint32_t voiceSpell = 0;
  uint32_t instantSpell = 0;

  nlohmann::json ToJson() const;

  static Equipment FromJson(const simdjson::dom::element& element);
};
