#include "Equipment.h"

bool Equipment::IsSpellEquipped(const uint32_t spellFormId) const
{
  return spellFormId == leftSpell || spellFormId == rightSpell ||
    spellFormId == voiceSpell || spellFormId == instantSpell;
}
