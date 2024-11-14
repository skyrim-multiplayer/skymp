#include "Equipment.h"
#include "archives/JsonInputArchive.h"
#include "archives/JsonOutputArchive.h"

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
  std::string minifiedDump = simdjson::minify(element);
  nlohmann::json j = nlohmann::json::parse(minifiedDump);

  JsonInputArchive ar(j);
  Equipment res;
  res.Serialize(ar);
  return res;
}
