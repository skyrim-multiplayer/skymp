#include "EspmGameObject.h"

EspmGameObject::EspmGameObject(const espm::LookupResult& record_)
  : record(record_)
{
}

/*
return t == "AMMO" || t == "ARMO" || t == "BOOK" || t == "INGR" ||
    t == "ALCH" || t == "SCRL" || t == "SLGM" || t == "WEAP" || t == "MISC";
*/
const char* EspmGameObject::GetParentNativeScript()
{
  if (record.rec) {
    auto t = record.rec->GetType();
    if (t == "AMMO")
      return "ammo";
    if (t == "ARMO")
      return "armor";
    if (t == "BOOK")
      return "book";
    if (t == "ALCH")
      return "potion";
    if (t == "SCRL")
      return "scroll";
    if (t == "SLGM")
      return "soulgem";
    if (t == "WEAP")
      return "weapon";
    if (t == "INGR")
      return "ingredient";
    if (t == "MISC")
      return "miscobject";
    if (t == "MESG")
      return "message";
    if (t == "FLST")
      return "formlist";

    throw std::runtime_error("Unable to find native script for record type '" +
                             t.ToString() + "'");
  }
  return "";
}

const espm::LookupResult& GetRecordPtr(const VarValue& papyrusObject)
{
  static const espm::LookupResult emptyResult;

  if (papyrusObject.GetType() != VarValue::kType_Object)
    return emptyResult;
  auto gameObject = static_cast<IGameObject*>(papyrusObject);
  auto espmGameObject = dynamic_cast<EspmGameObject*>(gameObject);
  if (!espmGameObject)
    return emptyResult;
  return espmGameObject->record;
}
