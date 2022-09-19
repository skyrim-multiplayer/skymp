#pragma once

#ifdef _SP_WITH_NETWORKING_CLIENT

#  include "Networking.h"
#  include <cstdint>

namespace NetworkingClient {
typedef void (*OnPacket)(int32_t type, const char* jsonContent,
                         const char* error, void* state_);

struct State
{
  std::shared_ptr<Networking::IClient> client;
  std::queue<Packet> queue;
};

void Create(const char* targetHostname, uint16_t targetPort);
void Destroy();
bool IsConnected();
void Tick(OnPacket onPacket, void* state_);
void Send(const char* jsonContent, bool reliable);
};

#endif
