#include "CropHealthRegeneration.h"
#include "GetBaseActorValues.h"
#include "MpActor.h"

float PercentToFloat(float percent)
{
  return percent / 100.0f;
}

float CropHealthRegeneration(float newDamageModifier,
                             float secondsAfterLastRegen, MpActor* actor)
{
  auto baseId = actor->GetBaseId();
  auto look = actor->GetLook();
  uint32_t raceId = look ? look->raceId : 0;
  auto baseValues = GetBaseActorValues(baseId, raceId);

  auto validHealthRegenerationPercentage =
    PercentToFloat(baseValues.healRate) *
    PercentToFloat(baseValues.healRateMult) * secondsAfterLastRegen;

  if (newDamageModifier > validHealthRegenerationPercentage)
    return validHealthRegenerationPercentage;
  if (newDamageModifier < 0)
    return 0;
  return newDamageModifier;
}
