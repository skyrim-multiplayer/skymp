#include "GetBaseActorValues.h"
#include "WorldState.h"

#include <spdlog/spdlog.h>

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
          std::to_string(changeForm.actorValues.healthPercentage).c_str());
  visitor("staminaPercentage",
          std::to_string(changeForm.actorValues.staminaPercentage).c_str());
  visitor("magickaPercentage",
          std::to_string(changeForm.actorValues.magickaPercentage).c_str());
}

BaseActorValues GetBaseActorValues(WorldState* worldState, uint32_t baseId,
                                   uint32_t raceIdOverride)
{
  auto npcData = espm::GetData<espm::NPC_>(baseId, worldState);
  uint32_t raceID = raceIdOverride ? raceIdOverride : npcData.race;
  auto raceData = espm::GetData<espm::RACE>(raceID, worldState);

  BaseActorValues actorValues;

  actorValues.health = raceData.startingHealth + npcData.healthOffset;
  if (actorValues.health <= 0) {
    spdlog::warn("GetBaseActorValues {:x} {:x} - Negative Health found: "
                 "startingHealth={}, healthOffset={}, defaulting to 100",
                 baseId, raceIdOverride, raceData.startingHealth,
                 npcData.healthOffset);
    actorValues.health = 100.f;
  }

  actorValues.magicka = raceData.startingMagicka + npcData.magickaOffset;
  if (actorValues.magicka <= 0) {
    spdlog::warn("GetBaseActorValues {:x} {:x} - Negative Magicka found: "
                 "startingMagicka={}, magickaOffset={}, defaulting to 100",
                 baseId, raceIdOverride, raceData.startingMagicka,
                 npcData.magickaOffset);
    actorValues.magicka = 100.f;
  }

  actorValues.stamina = raceData.startingStamina + npcData.staminaOffset;
  if (actorValues.stamina <= 0) {
    spdlog::warn("GetBaseActorValues {:x} {:x} - Negative Stamina found: "
                 "startingStamina={}, staminaOffset={}, defaulting to 100",
                 baseId, raceIdOverride, raceData.startingStamina,
                 npcData.staminaOffset);
    actorValues.stamina = 100.f;
  }

  actorValues.healRate = raceData.healRegen;
  actorValues.magickaRate = raceData.magickaRegen;
  actorValues.staminaRate = raceData.staminaRegen;

  spdlog::trace(
    "GetBaseActorValues {:x} {:x} - startingHealth={}, healthOffset={}",
    baseId, raceIdOverride, raceData.startingHealth, npcData.healthOffset);

  return actorValues;
}
