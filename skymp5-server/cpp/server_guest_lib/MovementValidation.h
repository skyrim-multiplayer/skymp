#pragma once
#include "FormDesc.h"
#include "NiPoint3.h"
#include "PartOne.h"
#include <cstdint>
#include <string>
#include <vector>

namespace MovementValidation {
bool Validate(const NiPoint3& currentPos, const NiPoint3& currentRot,
              const FormDesc& currentCellOrWorld, const NiPoint3& newPos,
              const FormDesc& newCellOrWorld, Networking::UserId userId,
              PartOneSendTargetWrapper& sendTarget,
              const std::vector<std::string>& espmFiles);
}
