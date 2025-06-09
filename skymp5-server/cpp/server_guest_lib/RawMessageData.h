#pragma once
#include "NetworkingInterface.h"
#include <cstddef>
#include <cstdint>
#include <simdjson.h>

struct RawMessageData
{
  Networking::PacketData unparsed = nullptr;
  size_t unparsedLength = 0;
  simdjson::dom::element parsed;
  Networking::UserId userId = Networking::InvalidUserId;
};
