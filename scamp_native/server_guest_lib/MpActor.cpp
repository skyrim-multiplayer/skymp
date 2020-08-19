#include "MpActor.h"
#include "WorldState.h"
#include <NiPoint3.h>

void MpActor::SetRaceMenuOpen(bool isOpen)
{
  isRaceMenuOpen = isOpen;
}

void MpActor::SetLook(const Look* newLook)
{
  jLookCache.clear();
  if (newLook) {
    look.reset(new Look(*newLook));
  } else {
    look.reset();
  }
}

void MpActor::SetEquipment(const std::string& jsonString)
{
  jEquipmentCache = jsonString;
}

void MpActor::SendToUser(const void* data, size_t size, bool reliable)
{
  if (sendToUser)
    sendToUser(this, data, size, reliable);
  else
    throw std::runtime_error("sendToUser is nullptr");
}

void MpActor::AddEventSink(std::shared_ptr<DestroyEventSink> sink)
{
  destroyEventSinks.insert(sink);
}

void MpActor::RemoveEventSink(std::shared_ptr<DestroyEventSink> sink)
{
  destroyEventSinks.erase(sink);
}

MpActor::Tint MpActor::Tint::FromJson(simdjson::dom::element& j)
{
  Tint res;
  ReadEx(j, "argb", &res.argb);
  ReadEx(j, "type", &res.type);

  const char* texturePathCstr = "";
  ReadEx(j, "texturePath", &texturePathCstr);
  res.texturePath = texturePathCstr;

  return res;
}

MpActor::Look MpActor::Look::FromJson(const nlohmann::json& j)
{
  simdjson::dom::parser p;
  simdjson::dom::element parsed = p.parse(j.dump());
  return FromJson(parsed);
}

MpActor::Look MpActor::Look::FromJson(simdjson::dom::element& j)
{
  Look res;
  ReadEx(j, "isFemale", &res.isFemale);
  ReadEx(j, "raceId", &res.raceId);
  ReadEx(j, "weight", &res.weight);
  ReadEx(j, "skinColor", &res.skinColor);
  ReadEx(j, "hairColor", &res.hairColor);
  ReadVector(j, "headpartIds", &res.headpartIds);
  ReadEx(j, "headTextureSetId", &res.headTextureSetId);
  ReadVector(j, "options", &res.faceMorphs);
  ReadVector(j, "presets", &res.facePresets);

  const char* name;
  try {
    Read(j, "name", &name);
  } catch (std::exception&) {
    name = "";
  }
  res.name = name;

  simdjson::dom::element jTints;
  ReadEx(j, "tints", &jTints);

  res.tints.reserve(30);
  auto jTintsArr = jTints.operator simdjson::dom::array();
  for (simdjson::dom::element el : jTintsArr) {
    res.tints.push_back(Tint::FromJson(el));
  }

  return res;
}

std::string MpActor::Look::ToJson() const
{
  auto j = nlohmann::json{ { "isFemale", isFemale },
                           { "raceId", raceId },
                           { "weight", weight },
                           { "skinColor", skinColor },
                           { "hairColor", hairColor },
                           { "headpartIds", headpartIds },
                           { "headTextureSetId", headTextureSetId },
                           { "options", faceMorphs },
                           { "presets", facePresets },
                           { "name", name } };
  j["tints"] = nlohmann::json::array();
  for (auto& tint : tints) {
    j["tints"].push_back(nlohmann::json{ { "texturePath", tint.texturePath },
                                         { "argb", tint.argb },
                                         { "type", tint.type } });
  }
  return j.dump();
}

const std::string& MpActor::GetLookAsJson()
{
  if (look && jLookCache.empty())
    jLookCache = look->ToJson();
  return jLookCache;
}

void MpActor::UnsubscribeFromAll()
{
  auto emittersCopy = GetEmitters();
  for (auto emitter : emittersCopy)
    if (emitter != this)
      Unsubscribe(emitter, this);
}

void MpActor::BeforeDestroy()
{
  for (auto& sink : destroyEventSinks)
    sink->BeforeDestroy(*this);

  MpObjectReference::BeforeDestroy();

  UnsubscribeFromAll();
}