#pragma once
#include "IMessageOutput.h"
#include "IWorldObject.h"
#include <cstdint>

namespace MovementValidation {
bool Validate(const IWorldObject& worldObject, const NiPoint3& newPos,
              uint32_t newCellOrWorld, IMessageOutput& tgt);
}
