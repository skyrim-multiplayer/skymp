#include "EspmGameObject.h"

EspmGameObject::EspmGameObject(const espm::LookupResult& record_)
  : record(record_)
{
}

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
    if (t == "IDLE")
      return "idle";
    if (t == "ACTI")
      return "activator";

    // there is some code outside espm that relies on these types
    if (t == "REFR")
      return "objectreference";
    if (t == "ACHR")
      return "actor";

    // TODO: warning or proper handling
    // throw std::runtime_error("Unable to find native script for record type
    // '" +
    //                         t.ToString() + "'");
  }
  return "";
}

bool EspmGameObject::EqualsByValue(const IGameObject& obj) const
{
  if (auto espmObj = dynamic_cast<const EspmGameObject*>(&obj)) {
    return espmObj->record.rec == record.rec;
  }
  return false;
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
