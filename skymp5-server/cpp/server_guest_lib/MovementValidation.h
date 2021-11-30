#pragma once
#include "IMessageOutput.h"
#include "IWorldObject.h"
#include <cstdint>
#include <string>
#include <vector>

namespace MovementValidation {
bool Validate(const IWorldObject& worldObject, const NiPoint3& newPos,
              const FormDesc& newCellOrWorld, IMessageOutput& tgt,
              const std::vector<std::string>& espmFiles);
}
