#include "EspmGameObject.h"

EspmGameObject::EspmGameObject(const espm::LookupResult& record_)
  : record(record_)
{
}

const char* EspmGameObject::GetParentNativeScript()
{
  if (record.rec) {
    auto t = record.rec->GetType();
    if (t == "INGR")
      return "ingredient";
    if (t == "MISC")
      return "miscobject";
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
