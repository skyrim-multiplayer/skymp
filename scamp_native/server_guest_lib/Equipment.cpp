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
  simdjson::dom::element inv;
  ReadEx(element, "inv", &inv);

  uint32_t numChanges;
  try {
    ReadEx(element, "numChanges", &numChanges);
  } catch (std::exception& e) {
    numChanges = 0;
  }

  Equipment res;
  res.inv = Inventory::FromJson(inv);
  res.numChanges = numChanges;
  return res;
}