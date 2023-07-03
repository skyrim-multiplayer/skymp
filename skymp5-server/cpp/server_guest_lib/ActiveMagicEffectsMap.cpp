#include "ActiveMagicEffectsMap.h"
#include "JsonUtils.h"
#include "LeveledListUtils.h"
#include "TimeUtils.h"
#include "libespm/espm.h"
#include <utility>

std::optional<std::reference_wrapper<const ActiveMagicEffectsMap::Entry>>
ActiveMagicEffectsMap::Get(espm::ActorValue actorValue) const noexcept
{
  auto it = effects.find(actorValue);
  return it != effects.end() ? std::make_optional(it->second) : std::nullopt;
}

void ActiveMagicEffectsMap::Remove(espm::ActorValue actorValue) noexcept
{
  auto it = effects.find(actorValue);
  if (it != effects.end()) {
    effects.erase(it);
  }
}

void ActiveMagicEffectsMap::Clear() noexcept
{
  effects.clear();
}

ActiveMagicEffectsMap ActiveMagicEffectsMap::FromJson(
  const simdjson::dom::array& effects)
{
  static const JsonPointer effectId("effectId"), endTime("endTime"),
    duration("duration"), magnitude("magnitude"), areaOfEffect("areaOfEffect"),
    actorValue("actorValue");
  ActiveMagicEffectsMap res;
  for (const simdjson::dom::element& effect : effects) {
    Entry entry;
    std::string endTime;
    ReadEx(effect, endTime, &endTime);
    entry.endTime = TimeUtils::SystemTimeFrom(endTime);
    auto now = std::chrono::system_clock::now();
    if (now > entry.endTime) {
      continue;
    }
    int32_t av;
    ReadEx(effect, actorValue, &av);
    ReadEx(effect, effectId, &entry.data.effectId);
    ReadEx(effect, duration, &entry.data.areaOfEffect);
    ReadEx(effect, magnitude, &entry.data.magnitude);
    ReadEx(effect, areaOfEffect, &entry.data.areaOfEffect);
    res.Add(static_cast<espm::ActorValue>(av), std::move(entry));
  }
  return res;
}

nlohmann::json::array_t ActiveMagicEffectsMap::ToJson() const
{
  auto res = nlohmann::json::array();
  for (const auto& [actorValue, effectEntry] : effects) {
    auto obj = nlohmann::json::object();
    obj["effectId"] = effectEntry.data.effectId;
    obj["endTime"] = TimeUtils::ToString(effectEntry.endTime);
    obj["magnitude"] = effectEntry.data.magnitude;
    obj["duration"] = effectEntry.data.duration;
    obj["areaOfEffect"] = effectEntry.data.areaOfEffect;
    obj["actorValue"] = static_cast<int32_t>(actorValue);
    res.push_back(obj);
  }
  return res;
}

std::vector<espm::Effects::Effect> ActiveMagicEffectsMap::GetActive()
  const noexcept
{
  std::vector<espm::Effects::Effect> activeEffects;
  activeEffects.reserve(effects.size());
  for (const auto [_, effectEntry] : effects) {
    activeEffects.push_back(effectEntry.data);
  }
  return activeEffects;
}

bool ActiveMagicEffectsMap::Has(espm::ActorValue actorValue) const noexcept
{
  auto it = effects.find(actorValue);
  return it != effects.end() ? true : false;
}
