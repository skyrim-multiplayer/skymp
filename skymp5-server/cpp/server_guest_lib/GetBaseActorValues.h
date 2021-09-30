#pragma once
#include <Loader.h>
#include <WorldState.h>
#include <cstdint>
#include <espm.h>
#include <fmt/format.h>

struct BaseActorValues
{
  float health = 100.f;
  float stamina = 100.f;
  float magicka = 100.f;
  float healRate = 0.7f;
  float staminaRate = 5.0f;
  float magickaRate = 3.0f;
  float healRateMult = 100.f;
  float staminaRateMult = 100.f;
  float magickaRateMult = 100.f;

  using PropertiesVisitor =
    std::function<void(const char* propName, const char* jsonValue)>;

  void VisitBaseActorValues(BaseActorValues& baseActorValues,
                            MpChangeForm& changeForm,
                            const PropertiesVisitor& visitor)
  {
    visitor("health", std::to_string(baseActorValues.health).c_str());
    visitor("stamina", std::to_string(baseActorValues.stamina).c_str());
    visitor("magicka", std::to_string(baseActorValues.magicka).c_str());
    visitor("healRate", std::to_string(baseActorValues.healRate).c_str());
    visitor("staminaRate",
            std::to_string(baseActorValues.staminaRate).c_str());
    visitor("magickaRate",
            std::to_string(baseActorValues.magickaRate).c_str());
    visitor("healRateMult",
            std::to_string(baseActorValues.healRateMult).c_str());
    visitor("staminaRateMult",
            std::to_string(baseActorValues.staminaRateMult).c_str());
    visitor("magickaRateMult",
            std::to_string(baseActorValues.magickaRateMult).c_str());
    visitor("healthPercentage",
            std::to_string(changeForm.healthPercentage).c_str());
    visitor("staminaPercentage",
            std::to_string(changeForm.staminaPercentage).c_str());
    visitor("magickaPercentage",
            std::to_string(changeForm.magickaPercentage).c_str());
  }
};

namespace {
void ExtractBaseActorValues(const espm::LookupResult& result,
                            espm::CompressedFieldsCache& compressedFieldsCache,
                            BaseActorValues& baseActorValues)
{
  auto race = espm::Convert<espm::RACE>(result.rec);

  auto raceData = race->GetData(compressedFieldsCache);

  baseActorValues.health = raceData.startingHealth;
  baseActorValues.magicka = raceData.startingMagicka;
  baseActorValues.stamina = raceData.startingStamina;
  baseActorValues.healRate = raceData.healRegen;
  baseActorValues.magickaRate = raceData.magickaRegen;
  baseActorValues.staminaRate = raceData.staminaRegen;
}
}

inline BaseActorValues GetBaseActorValues(espm::Loader& espm, uint32_t baseId,
                                          uint32_t raceIdOverride)
{
  BaseActorValues baseActorValues;
  espm::CompressedFieldsCache compressedFieldsCache;

  if (raceIdOverride) {
    auto raceInfo = espm.GetBrowser().LookupById(raceIdOverride);

    if (raceInfo.rec->GetType() == "RACE") {
      ExtractBaseActorValues(raceInfo, compressedFieldsCache, baseActorValues);
    } else {
      std::string errorMessage = fmt::format(
        "Unable to read RACE. formId: {}, raceId: {}", baseId, raceIdOverride);
      throw std::runtime_error(errorMessage);
    }
  } else {
    auto form = espm.GetBrowser().LookupById(baseId);
    if (form.rec->GetType() == "NPC_") {
      auto npc = espm::Convert<espm::NPC_>(form.rec);
      auto raceId = npc->GetData(compressedFieldsCache).race;
      auto raceInfo = espm.GetBrowser().LookupById(raceId);
      ExtractBaseActorValues(raceInfo, compressedFieldsCache, baseActorValues);
    } else {
      std::string errorMessage =
        fmt::format("Unable to read NPC_. formId: {}", baseId);
      throw std::runtime_error(errorMessage);
    }
  }
  return baseActorValues;
}
