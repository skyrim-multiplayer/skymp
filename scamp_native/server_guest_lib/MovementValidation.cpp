#include "MovementValidation.h"
#include "NetworkingInterface.h"
#include "NiPoint3.h"
#include <nlohmann/json.hpp>
#include "BinaryStructures.h"
#include <string>

bool MovementValidation::Validate(const IWorldObject& worldObject,
                                  const NiPoint3& newPos,
                                  uint32_t newCellOrWorld, IMessageOutput& tgt)
{
  const auto& currentPos = worldObject.GetPos();
  const auto& currentRot = worldObject.GetAngle();
  const auto& currentCellOrWorld = worldObject.GetCellOrWorld();

  float maxDistance = 4096;
  if (currentCellOrWorld != newCellOrWorld ||
      (currentPos - newPos).Length() >= maxDistance) {
    // a long time ago, in far, far legacy, there was a player movement correction using teleport
    return false;
  }
  return true;
}