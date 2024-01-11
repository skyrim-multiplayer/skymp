#include "GetBaseActorValues.h"
#include "EvaluateTemplate.h"
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

// TODO: implement auto-calc flag
BaseActorValues GetBaseActorValues(WorldState* worldState, uint32_t baseId,
                                   uint32_t raceIdOverride,
                                   const std::vector<FormDesc>& templateChain)
{

  auto npcData = espm::GetData<espm::NPC_>(baseId, worldState);

  uint32_t raceId = raceIdOverride
    ? raceIdOverride
    : EvaluateTemplate<espm::NPC_::UseTraits>(
        worldState, baseId, templateChain,
        [](const auto& npcLookupResult, const auto& npcData) {
          return npcLookupResult.ToGlobalId(npcData.race);
        });
  auto raceData = espm::GetData<espm::RACE>(raceId, worldState);

  espm::NPC_::Data attributesNpcData = EvaluateTemplate<espm::NPC_::UseStats>(
    worldState, baseId, templateChain,
    [](const auto&, const auto& npcData) { return npcData; });

  BaseActorValues actorValues;

  actorValues.health =
    raceData.startingHealth + attributesNpcData.healthOffset;
  if (actorValues.health <= 0) {
    spdlog::warn("GetBaseActorValues {:x} {:x} - Negative Health found: "
                 "startingHealth={}, healthOffset={}, defaulting to 100",
                 baseId, raceIdOverride, raceData.startingHealth,
                 attributesNpcData.healthOffset);
    actorValues.health = 100.f;
  }

  actorValues.magicka =
    raceData.startingMagicka + attributesNpcData.magickaOffset;
  if (actorValues.magicka < 0) { // zero magicka is ok, negative isn't
    spdlog::warn("GetBaseActorValues {:x} {:x} - Negative Magicka found: "
                 "startingMagicka={}, magickaOffset={}, defaulting to 100",
                 baseId, raceIdOverride, raceData.startingMagicka,
                 attributesNpcData.magickaOffset);
    actorValues.magicka = 100.f;
  }

  actorValues.stamina =
    raceData.startingStamina + attributesNpcData.staminaOffset;
  if (actorValues.stamina <= 0) {
    spdlog::warn("GetBaseActorValues {:x} {:x} - Negative Stamina found: "
                 "startingStamina={}, staminaOffset={}, defaulting to 100",
                 baseId, raceIdOverride, raceData.startingStamina,
                 attributesNpcData.staminaOffset);
    actorValues.stamina = 100.f;
  }

  actorValues.healRate = raceData.healRegen;
  actorValues.magickaRate = raceData.magickaRegen;
  actorValues.staminaRate = raceData.staminaRegen;

  spdlog::trace(
    "GetBaseActorValues {:x} {:x} - startingHealth={}, healthOffset={}",
    baseId, raceIdOverride, raceData.startingHealth,
    attributesNpcData.healthOffset);

  return actorValues;
}
