#include "MpClientPlugin.h"

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
      auto [onPacket, state_] =
        *reinterpret_cast<std::pair<OnPacket, void*>*>(state);

      auto contentPtr = "";

      if (packetType == Networking::PacketType::Message && length > 1) {
        std::string jsonContent(reinterpret_cast<const char*>(data) + 1,
                                length - 1);
        contentPtr = jsonContent.data();
      }

      onPacket((int32_t)packetType, contentPtr, error, state_);
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