#pragma once

#include <slikenet/types.h>
#include <nlohmann/json_fwd.hpp>

#include "MovementData.h"

//namespace SLNet {
//class RAK_DLL_EXPORT BitStream;
//}

void Serialize(const MovementData& movData, SLNet::BitStream& stream, bool isWrite);
