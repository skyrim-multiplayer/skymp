#include "VoiceChat.h"
#include "archives/JsonOutputArchive.h"
#include "archives/SimdJsonInputArchive.h"

nlohmann::json VoiceChat::ToJson() const
{
  JsonOutputArchive ar;
  const_cast<VoiceChat*>(this)->Serialize(ar);
  return std::move(ar.j);
}

VoiceChat VoiceChat::FromJson(const simdjson::dom::element& element)
{
  SimdJsonInputArchive ar(element);
  VoiceChat res;
  res.Serialize(ar);
  return res;
}
