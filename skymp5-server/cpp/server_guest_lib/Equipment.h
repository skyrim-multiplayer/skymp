#pragma once
#include "Inventory.h"

struct Equipment
{
  [[nodiscard]] bool IsSpellEquipped(uint32_t spellFormId) const;

  // TODO: get rid in favor of Serialize
  nlohmann::json ToJson() const;
  static Equipment FromJson(const simdjson::dom::element& element);

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("inv", inv)
      .Serialize("leftSpell", leftSpell)
      .Serialize("rightSpell", rightSpell)
      .Serialize("voiceSpell", voiceSpell)
      .Serialize("instantSpell", instantSpell)
      .Serialize("numChanges", numChanges);
  }

  Inventory inv;
  std::optional<uint32_t> leftSpell;
  std::optional<uint32_t> rightSpell;
  std::optional<uint32_t> voiceSpell;
  std::optional<uint32_t> instantSpell;
  uint32_t numChanges = 0;
};
