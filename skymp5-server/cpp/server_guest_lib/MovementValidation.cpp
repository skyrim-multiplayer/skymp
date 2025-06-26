#include "MovementValidation.h"
#include "FormDesc.h"
#include "NiPoint3.h"
#include <nlohmann/json.hpp>
#include <string>
#include "TeleportMessage.h"
#include "PartOne.h"

namespace MovementValidation {

bool Validate(const NiPoint3& currentPos,
              const NiPoint3& currentRot,
              const FormDesc& currentCellOrWorld,
              const NiPoint3& newPos,
              const FormDesc& newCellOrWorld,
              Networking::UserId userId,
              PartOneSendTargetWrapper& sendTarget,
              const std::vector<std::string>& espmFiles)
{
  float maxDistance = 4096;
  if (currentCellOrWorld != newCellOrWorld ||
      (currentPos - newPos).Length() >= maxDistance) {
    TeleportMessage msg;
    msg.pos = { newPos[0], newPos[1], newPos[2] };
    msg.rot = { currentRot[0], currentRot[1], currentRot[2] };
    msg.worldOrCell = currentCellOrWorld.ToFormId(espmFiles);
    sendTarget.Send(userId, msg, true);
    return false;
  }
  return true;
}

} // namespace MovementValidation
