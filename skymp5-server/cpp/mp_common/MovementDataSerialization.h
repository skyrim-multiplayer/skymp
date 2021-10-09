#pragma once

#include <slikenet/types.h>
#include <nlohmann/json_fwd.hpp>

#include "MovementData.h"

// namespace serialization {

void WriteToBitStream(SLNet::BitStream& stream, const MovementData& movData);
void ReadFromBitStream(SLNet::BitStream& stream, MovementData& movData);

MovementData MovementDataFromJson(const nlohmann::json& json);
nlohmann::json MovementDataToJson(const MovementData& movData);

// }
