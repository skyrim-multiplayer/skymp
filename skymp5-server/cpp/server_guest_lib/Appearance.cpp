#include "Appearance.h"
#include "JsonUtils.h"
#include "archives/JsonInputArchive.h"
#include "archives/JsonOutputArchive.h"
#include "archives/SimdJsonInputArchive.h"

Tint Tint::FromJson(simdjson::dom::element& element)
{
  SimdJsonInputArchive ar(element);
  Tint res;
  res.Serialize(ar);
  return res;
}

Appearance Appearance::FromJson(const nlohmann::json& j)
{
  JsonInputArchive ar(j);
  Appearance res;
  res.Serialize(ar);
  return res;
}

Appearance Appearance::FromJson(simdjson::dom::element& j)
{
  SimdJsonInputArchive ar(element);
  Appearance res;
  res.Serialize(ar);
  return res;
}

std::string Appearance::ToJson() const
{
  JsonOutputArchive ar;
  const_cast<Appearance*>(this)->Serialize(ar);
  return ar.j.dump();
}
