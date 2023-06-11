#pragma once
#include "System.h"

class OnEquipSystem final
{
public:
  static void Run(WorldState& worldState, uint32_t formId,
                  uint32_t itemBaseId);
};