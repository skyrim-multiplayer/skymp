#include "Equipment.h"
#include "archives/JsonOutputArchive.h"
#include "archives/SimdJsonInputArchive.h"

bool Equipment::IsSpellEquipped(const uint32_t spellFormId) const
{
  return spellFormId == leftSpell || spellFormId == rightSpell ||
    spellFormId == voiceSpell || spellFormId == instantSpell;
}

nlohmann::json Equipment::ToJson() const
{
  JsonOutputArchive ar;
  const_cast<Equipment*>(this)->Serialize(ar);
  return std::move(ar.j);
}

Equipment Equipment::FromJson(const simdjson::dom::element& element)
{
  SimdJsonInputArchive ar(element);
  Equipment res;
  res.Serialize(ar);
  return res;
}
