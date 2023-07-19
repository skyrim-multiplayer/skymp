#include "GetBaseActorValues.h"
#include "WorldState.h"

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
  actorValues.magicka = raceData.startingMagicka + npcData.magickaOffset;
  actorValues.stamina = raceData.startingStamina + npcData.staminaOffset;
  actorValues.healRate = raceData.healRegen;
  actorValues.magickaRate = raceData.magickaRegen;
  actorValues.staminaRate = raceData.staminaRegen;
  return actorValues;
}
