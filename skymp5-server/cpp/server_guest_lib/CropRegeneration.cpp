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
  auto look = actor->GetLook();
  uint32_t raceId = look ? look->raceId : 0;
  BaseActorValues baseValues;
  auto* worldState = actor->GetParent();
  if (worldState && worldState->HasEspm()) {
    auto& espm = actor->GetParent()->GetEspm();
    baseValues = GetBaseActorValues(espm, baseId, raceId);
  }
  return baseValues;
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
                          actor->GetChangeForm().healthPercentage);
}

float CropMagickaRegeneration(float newAttributeValue,
                              float secondsAfterLastRegen, MpActor* actor)
{
  BaseActorValues baseValues = GetValues(actor);
  return CropRegeneration(newAttributeValue, secondsAfterLastRegen,
                          baseValues.magickaRate, baseValues.magickaRateMult,
                          actor->GetChangeForm().magickaPercentage);
}

float CropStaminaRegeneration(float newAttributeValue,
                              float secondsAfterLastRegen, MpActor* actor)
{
  BaseActorValues baseValues = GetValues(actor);
  return CropRegeneration(newAttributeValue, secondsAfterLastRegen,
                          baseValues.staminaRate, baseValues.staminaRateMult,
                          actor->GetChangeForm().staminaPercentage);
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
