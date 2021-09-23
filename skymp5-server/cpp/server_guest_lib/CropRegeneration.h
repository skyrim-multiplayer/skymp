#pragma once
#include "WorldState.h"
float CropHealthRegeneration(float newAttributeValue,
                             float secondsAfterLastRegen, MpActor* actor);

float CropMagickaRegeneration(float newAttributeValue,
                              float secondsAfterLastRegen, MpActor* actor);

float CropStaminaRegeneration(float newAttributeValue,
                              float secondsAfterLastRegen, MpActor* actor);
