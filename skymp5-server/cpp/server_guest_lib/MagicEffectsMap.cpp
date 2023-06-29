#include "MagicEffectsMap.h"
#include "JsonUtils.h"
#include "LeveledListUtils.h"
#include "TimeUtils.h"
#include "libespm/espm.h"
#include <utility>

std::optional<std::reference_wrapper<const MagicEffectsMap::Entry>>
MagicEffectsMap::Get(espm::ActorValue actorValue) const noexcept
{
  auto it = effects.find(actorValue);
  return it != effects.end() ? std::make_optional(it->second) : std::nullopt;
}

void MagicEffectsMap::Remove(espm::ActorValue actorValue) noexcept
{
  auto it = effects.find(actorValue);
  if (it != effects.end()) {
    effects.erase(it);
  }
}

void MagicEffectsMap::Clear() noexcept
{
  effects.clear();
}

MagicEffectsMap MagicEffectsMap::FromJson(const simdjson::dom::array& effects)
{
  static const JsonPointer effectId("effectId"), endTime("endTime"),
    duration("duration"), magnitude("magnitude"), areaOfEffect("areaOfEffect"),
    actorValue("actorValue");
  MagicEffectsMap res;
  for (const simdjson::dom::element& effect : effects) {
    Entry entry;
    std::string endTime;
    ReadEx(effect, endTime, &endTime);
    entry.endTime = TimeUtils::SystemTimeFrom(endTime);
    auto now = std::chrono::system_clock::now();
    if (now > entry.endTime) {
      continue;
    }
    espm::ActorValue av;
    ReadEx(effect, actorValue, &av);
    ReadEx(effect, effectId, &entry.data.effectId);
    ReadEx(effect, duration, &entry.data.areaOfEffect);
    ReadEx(effect, magnitude, &entry.data.magnitude);
    ReadEx(effect, areaOfEffect, &entry.data.areaOfEffect);
    res.Add(av, std::move(entry));
  }
  return res;
}

nlohmann::json::array_t MagicEffectsMap::ToJson() const
{
  auto res = nlohmann::json::array();
  for (const auto& [actorValue, effectEntry] : effects) {
    auto obj = nlohmann::json::object();
    obj["effectId"] = effectEntry.data.effectId;
    obj["endTime"] = TimeUtils::ToString(effectEntry.endTime);
    obj["magnitude"] = effectEntry.data.magnitude;
    obj["duration"] = effectEntry.data.duration;
    obj["areaOfEffect"] = effectEntry.data.areaOfEffect;
    obj["actorValue"] = actorValue;
    res.push_back(obj);
  }
  return res;
}

std::vector<espm::Effects::Effect> MagicEffectsMap::GetActive() const noexcept
{
  std::vector<espm::Effects::Effect> activeEffects;
  activeEffects.reserve(effects.size());
  for (const auto [_, effectEntry] : effects) {
    activeEffects.push_back(effectEntry.data);
  }
  return activeEffects;
}
