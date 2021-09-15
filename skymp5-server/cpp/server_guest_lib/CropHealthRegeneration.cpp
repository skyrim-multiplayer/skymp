#include "CropHealthRegeneration.h"
double CropHealthRegeneration(double newDamageModifier, int timeAfterLastRegen,
                              MpActor* actor)
{
  if (newDamageModifier >= 0 && newDamageModifier <= 1)
    return newDamageModifier;
  else
    return 1;
}