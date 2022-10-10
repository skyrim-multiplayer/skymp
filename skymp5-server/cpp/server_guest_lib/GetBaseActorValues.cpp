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

float BaseActorValues::GetValue(espm::ActorValue av)
{
  switch (av) {
    case espm::ActorValue::Health:
      return health;
    case espm::ActorValue::Magicka:
      return magicka;
    case espm::ActorValue::Stamina:
      return stamina;
    case espm::ActorValue::HealRate:
      return healRate;
    case espm::ActorValue::MagickaRate:
      return magickaRate;
    case espm::ActorValue::StaminaRate:
      return staminaRate;
    case espm::ActorValue::HealRateMult_or_CombatHealthRegenMultMod:
      return healRateMult;
    case espm::ActorValue::MagickaRateMult_or_CombatHealthRegenMultPowerMod:
      return magickaRateMult;
    case espm::ActorValue::StaminaRateMult:
      return staminaRateMult;
    default:
      throw std::runtime_error(
        fmt::format("Unsupported actor value type {:}", (int32_t)av));
  }
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
