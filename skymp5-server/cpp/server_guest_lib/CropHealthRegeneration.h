#pragma once
#include "Loader.h"
#include "WorldState.h"
float CropHealthRegeneration(espm::Loader& espm, float newDamageModifier,
                             float secondsAfterLastRegen, MpActor* actor);
