#include "MpActor.h"
#include "WorldState.h"
#include <NiPoint3.h>

namespace {
std::pair<int16_t, int16_t> GetGridPos(const NiPoint3& pos) noexcept
{
  return { int16_t(pos.x / 4096), int16_t(pos.y / 4096) };
}
}

void MpActor::SetPos(const NiPoint3& newPos)
{
  auto& grid = GetParent()->grids[cellOrWorld];

  auto oldGridPos = GetGridPos(pos);
  auto newGridPos = GetGridPos(newPos);
  if (oldGridPos != newGridPos || !isOnGrid) {
    grid.Move(this, newGridPos.first, newGridPos.second);
    isOnGrid = true;

    auto& was = this->listeners;
    auto& now = grid.GetNeighboursAndMe(this);

    std::vector<MpActor*> toRemove;
    std::set_difference(was.begin(), was.end(), now.begin(), now.end(),
                        std::inserter(toRemove, toRemove.begin()));
    for (auto listener : toRemove) {
      Unsubscribe(this, listener);
      if (listener != this)
        Unsubscribe(listener, this);
    }

    std::vector<MpActor*> toAdd;
    std::set_difference(now.begin(), now.end(), was.begin(), was.end(),
                        std::inserter(toAdd, toAdd.begin()));
    for (auto listener : toAdd) {
      Subscribe(this, listener);
      if (listener != this)
        Subscribe(listener, this);
    }
  }
  pos = newPos;
}

void MpActor::SetAngle(const NiPoint3& newAngle)
{
  rot = newAngle;
}

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

void MpActor::UnsubscribeFromAll()
{
  auto emittersCopy = emitters;
  for (auto emitter : emittersCopy)
    if (emitter != this)
      Unsubscribe(emitter, this);
}

void MpActor::BeforeDestroy()
{
  GetParent()->grids[cellOrWorld].Forget(this);

  auto listenersCopy = listeners;
  for (auto listener : listenersCopy)
    if (this != listener)
      Unsubscribe(this, listener);

  UnsubscribeFromAll();
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

void MpActor::Subscribe(MpActor* emitter, MpActor* listener)
{
  emitter->listeners.insert(listener);
  listener->emitters.insert(emitter);
  emitter->onSubscribe(emitter, listener);
}

void MpActor::Unsubscribe(MpActor* emitter, MpActor* listener)
{
  emitter->onUnsubscribe(emitter, listener);
  emitter->listeners.erase(listener);
  listener->emitters.erase(emitter);
}
