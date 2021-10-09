#pragma once

#include <slikenet/types.h>
#include <nlohmann/json_fwd.hpp>

#include "MovementData.h"

void Write(const MovementData& movData, SLNet::BitStream& stream);
void ReadTo(MovementData& movData, SLNet::BitStream& stream);

MovementData MovementDataFromJson(const nlohmann::json& json);
nlohmann::json MovementDataToJson(const MovementData& movData);
