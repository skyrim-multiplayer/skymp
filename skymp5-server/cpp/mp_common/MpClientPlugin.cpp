#include "MpClientPlugin.h"
#include <vector>

void MpClientPlugin::CreateClient(State& state, const char* targetHostname,
                                  uint16_t targetPort)
{
  state.cl = Networking::CreateClient(targetHostname, targetPort);
}

void MpClientPlugin::DestroyClient(State& state)
{
  state.cl.reset();
}

bool MpClientPlugin::IsConnected(State& state)
{
  return state.cl && state.cl->IsConnected();
}

void MpClientPlugin::Tick(State& state, OnPacket onPacket, void* state_)
{
  if (!state.cl)
    return;

  std::pair<OnPacket, void*> packetAndState(onPacket, state_);

  state.cl->Tick(
    [](void* state, Networking::PacketType packetType,
       Networking::PacketData data, size_t length, const char* error) {
      auto onPacketAndState =
        *reinterpret_cast<std::pair<OnPacket, void*>*>(state);

      std::string jsonContent;

      if (packetType == Networking::PacketType::Message && length > 1) {
        jsonContent =
          std::string(reinterpret_cast<const char*>(data) + 1, length - 1);
      }

      onPacketAndState.first((int32_t)packetType, jsonContent.data(), error,
                             onPacketAndState.second);
    },
    &packetAndState);
}

void MpClientPlugin::Send(State& state, const char* jsonContent, bool reliable)
{
  if (!state.cl)
    return;
  auto n = strlen(jsonContent);
  std::vector<uint8_t> buf(n + 1);
  buf[0] = Networking::MinPacketId;
  memcpy(buf.data() + 1, jsonContent, n);

  state.cl->Send(buf.data(), buf.size(), reliable);
}
