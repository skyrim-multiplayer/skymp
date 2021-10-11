#pragma once

#include <nlohmann/json_fwd.hpp>
#include <slikenet/types.h>

#include "MovementMessage.h"

namespace serialization {

void WriteToBitStream(SLNet::BitStream& stream,
                      const MovementMessage& movData);
void ReadFromBitStream(SLNet::BitStream& stream, MovementMessage& movData);

MovementMessage MovementMessageFromJson(const nlohmann::json& json);
nlohmann::json MovementMessageToJson(const MovementMessage& movData);

}
