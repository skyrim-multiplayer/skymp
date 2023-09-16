#pragma once
#include "Networking.h"
#include <cstdint>
#include <slikenet/types.h>

namespace MpClientPlugin {
typedef void (*OnPacket)(int32_t type, const char* jsonContent,
                         const char* error, void* state_);

struct State
{
  std::shared_ptr<Networking::IClient> cl;
};

typedef void(*SerializeMessage)(const char *jsonContent, SLNet::BitStream &outputBuffer);
typedef bool(*DeserializeMessage)(const uint8_t *data, size_t length, std::string &outJsonContent);

void CreateClient(State& st, const char* targetHostname, uint16_t targetPort);
void DestroyClient(State& st);
bool IsConnected(State& st);
void Tick(State& st, OnPacket onPacket, DeserializeMessage deserializeMessageFn, void* state_);
void Send(State& st, const char* jsonContent, bool reliable, SerializeMessage serializeMessageFn);
};
