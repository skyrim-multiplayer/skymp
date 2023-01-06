#include "CropRegeneration.h"
#include "GetBaseActorValues.h"
#include "MpActor.h"

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
                       float oldAttributeValue)
{
  float validRegenerationPercentage = PercentToFloat(attributeRate) *
    PercentToFloat(attributeRateMult) * secondsAfterLastRegen;
  validRegenerationPercentage =
    validRegenerationPercentage < 0.0f ? 0.0f : validRegenerationPercentage;

  float validAttributePercentage =
    oldAttributeValue + validRegenerationPercentage;
  validAttributePercentage =
    validAttributePercentage > 1.0f ? 1.0f : validAttributePercentage;

  if (newAttributeValue > validAttributePercentage) {
    return validAttributePercentage;
  }
  if (newAttributeValue < 0.0f) {
    return 0.0f;
  }
  return newAttributeValue;
}

float CropHealthRegeneration(float newAttributeValue,
                             float secondsAfterLastRegen, MpActor* actor)
{
  BaseActorValues baseValues = GetValues(actor);
  return CropRegeneration(newAttributeValue, secondsAfterLastRegen,
                          baseValues.healRate, baseValues.healRateMult,
                          actor->GetChangeForm().actorValues.healthPercentage);
}

float CropMagickaRegeneration(float newAttributeValue,
                              float secondsAfterLastRegen, MpActor* actor)
{
  BaseActorValues baseValues = GetValues(actor);
  return CropRegeneration(
    newAttributeValue, secondsAfterLastRegen, baseValues.magickaRate,
    baseValues.magickaRateMult,
    actor->GetChangeForm().actorValues.magickaPercentage);
}

float CropStaminaRegeneration(float newAttributeValue,
                              float secondsAfterLastRegen, MpActor* actor)
{
  BaseActorValues baseValues = GetValues(actor);
  return CropRegeneration(
    newAttributeValue, secondsAfterLastRegen,
    actor->GetChangeForm().actorValues.staminaRate, baseValues.staminaRateMult,
    actor->GetChangeForm().actorValues.staminaPercentage);
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
