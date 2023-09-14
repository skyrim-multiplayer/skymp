#pragma once
#include "Networking.h"
#include <cstdint>

namespace MpClientPlugin {
typedef void (*OnPacket)(int32_t type, const char* jsonContent,
                         const char* error, void* state_);

struct State
{
  std::shared_ptr<Networking::IClient> cl;
};

typedef void(*SerializeMessage)(const char *jsonContent, std::vector<uint8_t> &outputBuffer);

void CreateClient(State& st, const char* targetHostname, uint16_t targetPort);
void DestroyClient(State& st);
bool IsConnected(State& st);
void Tick(State& st, OnPacket onPacket, void* state_);
void Send(State& st, const char* jsonContent, bool reliable, SerializeMessage serializeMessageFn);
};
