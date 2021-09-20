#include "CropHealthRegeneration.h"
#include "GetBaseActorValues.h"
#include "MpActor.h"

namespace {
float PercentToFloat(float percent)
{
  return percent / 100.0f;
}
}

float CropHealthRegeneration(float newDamageModifier,
                             float secondsAfterLastRegen, MpActor* actor)
{
  uint32_t baseId = actor->GetBaseId();
  auto look = actor->GetLook();
  uint32_t raceId = look ? look->raceId : 0;
  BaseActorValues baseValues = GetBaseActorValues(baseId, raceId);

  float validHealthRegenerationPercentage =
    PercentToFloat(baseValues.healRate) *
    PercentToFloat(baseValues.healRateMult) * secondsAfterLastRegen;

  if (newDamageModifier > validHealthRegenerationPercentage) {
    return validHealthRegenerationPercentage;
  }
  if (newDamageModifier < 0) {
    return 0;
  }
  return newDamageModifier;
}
