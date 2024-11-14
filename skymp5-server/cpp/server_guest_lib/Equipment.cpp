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

// nlohmann::json Equipment::ToJson() const
// {
//   nlohmann::json res{
//     { "inv", inv.ToJson() },          { "leftSpell", leftSpell },
//     { "rightSpell", rightSpell },     { "voiceSpell", voiceSpell },
//     { "instantSpell", instantSpell }, { "numChanges", numChanges }
//   };

//   if (numChanges == 0)
//     res.erase("numChanges");
//   return res;
// }

// Equipment Equipment::FromJson(const simdjson::dom::element& element)
// {
//   static const JsonPointer inv("inv"), leftSpell("leftSpell"),
//     rightSpell("rightSpell"), voiceSpell("voiceSpell"),
//     instantSpell("instantSpell"), numChanges("numChanges");

//   simdjson::dom::element jInv;
//   ReadEx(element, inv, &jInv);

//   uint32_t numChangesValue;

//   if (element.at_pointer(numChanges.GetData()).error() ==
//       simdjson::error_code::SUCCESS) {
//     ReadEx(element, numChanges, &numChangesValue);
//   } else {
//     numChangesValue = 0;
//   }

//   uint32_t leftSpellValue = 0;

//   if (element.at_pointer(leftSpell.GetData()).error() ==
//       simdjson::error_code::SUCCESS) {
//     ReadEx(element, leftSpell, &leftSpellValue);
//   }

//   uint32_t rightSpellValue = 0;

//   if (element.at_pointer(rightSpell.GetData()).error() ==
//       simdjson::error_code::SUCCESS) {
//     ReadEx(element, rightSpell, &rightSpellValue);
//   }

//   uint32_t voiceSpellValue = 0;

//   if (element.at_pointer(voiceSpell.GetData()).error() ==
//       simdjson::error_code::SUCCESS) {
//     ReadEx(element, voiceSpell, &voiceSpellValue);
//   }

//   uint32_t instantSpellValue = 0;

//   if (element.at_pointer(instantSpell.GetData()).error() ==
//       simdjson::error_code::SUCCESS) {
//     ReadEx(element, instantSpell, &instantSpellValue);
//   }

//   Equipment res;
//   res.inv = Inventory::FromJson(jInv);
//   res.leftSpell = leftSpellValue;
//   res.rightSpell = rightSpellValue;
//   res.voiceSpell = voiceSpellValue;
//   res.instantSpell = instantSpellValue;
//   res.numChanges = numChangesValue;
//   return res;
// }
