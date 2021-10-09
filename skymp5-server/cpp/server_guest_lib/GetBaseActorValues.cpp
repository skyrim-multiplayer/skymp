#include "GetBaseActorValues.h"

void BaseActorValues::VisitBaseActorValues(BaseActorValues& baseActorValues,
                                           MpChangeForm& changeForm,
                                           const PropertiesVisitor& visitor)
{
  visitor("health", std::to_string(baseActorValues.health).c_str());
  visitor("stamina", std::to_string(baseActorValues.stamina).c_str());
  visitor("magicka", std::to_string(baseActorValues.magicka).c_str());
  visitor("healRate", std::to_string(baseActorValues.healRate).c_str());
  visitor("staminaRate", std::to_string(baseActorValues.staminaRate).c_str());
  visitor("magickaRate", std::to_string(baseActorValues.magickaRate).c_str());
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

BaseActorValues GetBaseActorValues(espm::Loader& espm, uint32_t baseId,
                                   uint32_t raceIdOverride)
{
  BaseActorValues baseActorValues;
  espm::CompressedFieldsCache compressedFieldsCache;
  uint32_t raceID = raceIdOverride;
  uint16_t healthOffset = 0;
  uint16_t magickaOffset = 0;
  uint16_t staminaOffset = 0;

  auto& browser = espm.GetBrowser();
  auto form = browser.LookupById(baseId);

  if (form.rec && form.rec->GetType() == "NPC_") {
    auto npcData =
      espm::Convert<espm::NPC_>(form.rec)->GetData(compressedFieldsCache);
    healthOffset = npcData.healthOffset;
    magickaOffset = npcData.magickaOffset;
    staminaOffset = npcData.staminaOffset;
    if (!raceIdOverride) {
      raceID = npcData.race;
    }
    auto raceInfo = browser.LookupById(raceID);

    if (raceInfo.rec && raceInfo.rec->GetType() == "RACE") {
      ExtractBaseActorValues(raceInfo, compressedFieldsCache, baseActorValues);
      baseActorValues.health += healthOffset;
      baseActorValues.magicka += magickaOffset;
      baseActorValues.stamina += staminaOffset;

    } else {
      std::string errorMessage = fmt::format(
        "Unable to read RACE. formId: {}, raceId: {}, raceidOcerride: {}",
        baseId, raceID, raceIdOverride);
      throw std::runtime_error(errorMessage);
    }
  } else {
    std::string errorMessage =
      fmt::format("Unable to read NPC_. formId: {}", baseId);
    throw std::runtime_error(errorMessage);
  }
  return baseActorValues;
}
