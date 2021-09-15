#include "CropHealthRegeneration.h"
float CropHealthRegeneration(float newDamageModifier,
                             float secondsAfterLastRegen, MpActor* actor)
{
  if (newDamageModifier >= 0.0f && newDamageModifier <= 1.0f) {
    return newDamageModifier;
  } else {
    return 0.0f;
  }
}
