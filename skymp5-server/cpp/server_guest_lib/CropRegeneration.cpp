#include "CropRegeneration.h"
#include "GetBaseActorValues.h"
#include "MathUtils.h"
#include "MpActor.h"
#include "MpChangeForms.h"

namespace {

float PercentToFloat(float percent)
{
  return percent / 100.0f;
}

BaseActorValues GetValues(MpActor* actor)
{
  uint32_t baseId = actor->GetBaseId();
  auto appearance = actor->GetAppearance();
  uint32_t raceId = appearance ? appearance->raceId : 0;
  auto worldState = actor->GetParent();
  return GetBaseActorValues(worldState, baseId, raceId);
}

}

float CropRegeneration(float newAttributeValue, float secondsAfterLastRegen,
                       float attributeRate, float attributeRateMult,
                       float oldAttributeValue, bool hasActiveMagicEffects)
{
  float validRegenerationPercentage = PercentToFloat(attributeRate) *
    PercentToFloat(attributeRateMult) * secondsAfterLastRegen;
  validRegenerationPercentage =
    validRegenerationPercentage < 0.0f ? 0.0f : validRegenerationPercentage;
  float validAttributePercentage =
    oldAttributeValue + validRegenerationPercentage;
  validAttributePercentage =
    validAttributePercentage > 1.0f ? 1.0f : validAttributePercentage;
  constexpr float kMaxOldPercentage = 1.f;

  if (newAttributeValue > validAttributePercentage) {
    return validAttributePercentage;
  }
  if (newAttributeValue < 0.0f) {
    return 0.0f;
  }
  if (hasActiveMagicEffects &&
      !MathUtils::IsNearlyEqual(oldAttributeValue, kMaxOldPercentage)) {
    return validAttributePercentage;
  }
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
  const float rate = std::max(baseValues.staminaRate, actorValues.staminaRate);
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
