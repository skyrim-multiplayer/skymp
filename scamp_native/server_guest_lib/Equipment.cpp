#include "Equipment.h"
#include "JsonUtils.h"

nlohmann::json Equipment::ToJson() const
{
  nlohmann::json res{ { "inv", static_cast<nlohmann::json>(inv.ToJson()) },
                      { "numChanges", numChanges } };
  if (numChanges == 0)
    res.erase("numChanges");
  return res;
}

Equipment Equipment::FromJson(simdjson::dom::element& element)
{
  static const JsonPointer inv("inv"), numChanges("numChanges");

  simdjson::dom::element jInv;
  ReadEx(element, inv, &jInv);

  uint32_t numChangesValue;
  try {
    ReadEx(element, numChanges, &numChangesValue);
  } catch (std::exception& e) {
    numChangesValue = 0;
  }

  Equipment res;
  res.inv = Inventory::FromJson(jInv);
  res.numChanges = numChangesValue;
  return res;
}