#pragma once
#include "FormDesc.h"
#include "IMessageOutput.h"
#include "NiPoint3.h"
#include <cstdint>
#include <string>
#include <vector>

namespace MovementValidation {
bool Validate(const NiPoint3& currentPos, const NiPoint3& currentRot,
              const FormDesc& currentCellOrWorld, const NiPoint3& newPos,
              const FormDesc& newCellOrWorld, IMessageOutput& tgt,
              const std::vector<std::string>& espmFiles);
}
