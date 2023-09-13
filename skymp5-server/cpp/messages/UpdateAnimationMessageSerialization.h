#pragma once

#include <nlohmann/json_fwd.hpp>
#include <slikenet/types.h>

#include "UpdateAnimationMessage.h"

namespace serialization {

void WriteToBitStream(SLNet::BitStream& stream,
                      const UpdateAnimationMessage& message);
void ReadFromBitStream(SLNet::BitStream& stream, UpdateAnimationMessage& message);

UpdateAnimationMessage UpdateAnimationMessageFromJson(const nlohmann::json& json);
nlohmann::json UpdateAnimationMessageToJson(const UpdateAnimationMessage& message);
}
