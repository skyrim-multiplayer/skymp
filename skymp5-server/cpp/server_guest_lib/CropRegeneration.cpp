#include "CropRegeneration.h"
#include "GetBaseActorValues.h"
#include "MathUtils.h"
#include "MpActor.h"
#include "MpChangeForms.h"

namespace {

BaseActorValues GetValues(MpActor* actor)
{
  uint32_t baseId = actor->GetBaseId();
  auto appearance = actor->GetAppearance();
  uint32_t raceId = appearance ? appearance->raceId : 0;
  auto worldState = actor->GetParent();
  return GetBaseActorValues(worldState, baseId, raceId,
                            actor->GetTemplateChain());
}

}

float CropRegeneration(float newAttributeValue, float secondsAfterLastRegen,
                       float attributeRate, float attributeRateMult,
                       float oldAttributeValue, bool hasActiveMagicEffects)
{
  spdlog::trace(
    "[crop]: args=(newAttributeValue={}, secondsAfterLastRegen={}, "
    "attributerate={}, attributeRateMult={}, oldAttributeValue={}, "
    "hasActiveMagicEffects={})",
    newAttributeValue, secondsAfterLastRegen, attributeRate, attributeRateMult,
    oldAttributeValue, hasActiveMagicEffects);

  float validRegenerationPercentage =
    MathUtils::PercentToFloat(attributeRate) *
    MathUtils::PercentToFloat(attributeRateMult) * secondsAfterLastRegen;

  spdlog::trace("[crop]: validRegenerationPercentage={}",
                validRegenerationPercentage);

  validRegenerationPercentage =
    validRegenerationPercentage < 0.0f ? 0.0f : validRegenerationPercentage;
  float validAttributePercentage =
    oldAttributeValue + validRegenerationPercentage;

  spdlog::trace("[crop]: validAttributePercentage={}",
                validAttributePercentage);

  validAttributePercentage =
    validAttributePercentage > 1.0f ? 1.0f : validAttributePercentage;
  constexpr float kMaxOldPercentage = 1.f;

  spdlog::trace("[crop]: comparing received attribute value and valid one: "
                "newAttributeValue={}, validAttributePercentage={}",
                newAttributeValue, validAttributePercentage);

  if (newAttributeValue > validAttributePercentage) {
    return validAttributePercentage;
  }
  if (newAttributeValue < 0.0f) {
    return 0.0f;
  }
  // if (hasActiveMagicEffects &&
  //    !MathUtils::IsNearlyEqual(oldAttributeValue, kMaxOldPercentage)) {
  //  return validAttributePercentage;
  // }
  return newAttributeValue;
}

float CropHealthRegeneration(float newAttributeValue,
                             float secondsAfterLastRegen, MpActor* actor)
{
  MpChangeForm changeForm = actor->GetChangeForm();
  const BaseActorValues baseValues = GetValues(actor);
  const ActorValues& actorValues = actor->GetChangeForm().actorValues;
  const float rate = std::max(baseValues.healRate, actorValues.healRate);
  const float rateMult =
    std::max(baseValues.healRateMult, actorValues.healRateMult);
  const float oldPercentage = actorValues.healthPercentage;
  const bool hasActiveMagicEffects = !changeForm.activeMagicEffects.Empty();
  return CropRegeneration(newAttributeValue, secondsAfterLastRegen, rate,
                          rateMult, oldPercentage, hasActiveMagicEffects);
}

float CropMagickaRegeneration(float newAttributeValue,
                              float secondsAfterLastRegen, MpActor* actor)
{
  MpChangeForm changeForm = actor->GetChangeForm();
  const BaseActorValues baseValues = GetValues(actor);
  const ActorValues& actorValues = changeForm.actorValues;
  const float rate = std::max(baseValues.magickaRate, actorValues.magickaRate);
  const float rateMult =
    std::max(baseValues.magickaRateMult, actorValues.magickaRateMult);
  const float oldPercentage = actorValues.magickaPercentage;
  const bool hasActiveMagicEffects = !changeForm.activeMagicEffects.Empty();
  return CropRegeneration(newAttributeValue, secondsAfterLastRegen, rate,
                          rateMult, oldPercentage, hasActiveMagicEffects);
}

float CropStaminaRegeneration(float newAttributeValue,
                              float secondsAfterLastRegen, MpActor* actor)
{
  const MpChangeForm changeForm = actor->GetChangeForm();
  const BaseActorValues baseValues = GetValues(actor);
  const ActorValues& actorValues = changeForm.actorValues;
  const float rate = actor->IsBlockActive()
    ? actorValues.staminaRate
    : std::max(baseValues.staminaRate, actorValues.staminaRate);
  const float rateMult =
    std::max(baseValues.staminaRateMult, actorValues.staminaRateMult);
  const float oldPercentage = actorValues.staminaPercentage;
  const bool hasActiveMagicEffects = !changeForm.activeMagicEffects.Empty();
  return CropRegeneration(newAttributeValue, secondsAfterLastRegen, rate,
                          rateMult, oldPercentage, hasActiveMagicEffects);
}

float CropPeriodAfterLastRegen(float secondsAfterLastRegen,
                               float maxValidPeriod, float defaultPeriod)
{
  if (secondsAfterLastRegen < 0.0f) {
    return 0.0f;
  }
  if (secondsAfterLastRegen > maxValidPeriod) {
    return defaultPeriod;
  }
  return secondsAfterLastRegen;
}

float CropValue(float value, float min, float max)
{
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}
