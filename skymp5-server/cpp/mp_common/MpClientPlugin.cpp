#include "MpClientPlugin.h"

#include <vector>

#include "MovementData.h"
#include "MovementDataSerialization.h"
#include "MsgType.h"

#include <nlohmann/json.hpp>

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
      const auto& [onPacket, state] = *reinterpret_cast<std::pair<OnPacket, void*>*>(state);

      std::string jsonContent;

      if (packetType == Networking::PacketType::Message && length > 1) {
        if (data[1] == MovementData::kHeaderByte) {
          MovementData movData;
          //                      v SLikeNet <3 <3 <3 <3 <3 <3 <3
          // (also, my code sucks too, but it's probably ok if I don't merge it)
          SLNet::BitStream stream(const_cast<unsigned char*>(data) + 2, length - 2, /*copyData*/false);
          ReadTo(movData, stream);
          jsonContent = MovementDataToJson(movData);
        } else {
          jsonContent =
            std::string(reinterpret_cast<const char*>(data) + 1, length - 1);
        }
      }

      onPacket(static_cast<int32_t>(packetType), jsonContent.data(), error,
                             state);
    },
    &packetAndState);
}

void MpClientPlugin::Send(State& state, const char* jsonContent, bool reliable)
{
  if (!state.cl) {
    // XXX: ???
    return;
  }

  const auto parsedJson = nlohmann::json::parse(jsonContent);
  if (static_cast<MsgType>(parsedJson.at("t").get<int>()) == MsgType::UpdateMovement) {
    const auto movData = MovementDataFromJson(parsedJson);
    SLNet::BitStream stream;
    Write(movData, stream);

    std::vector<uint8_t> buf(stream.GetNumberOfBytesUsed() + 2);
    buf[0] = Networking::MinPacketId;
    buf[1] = MovementData::kHeaderByte;
    std::copy(stream.GetData(), stream.GetData() + stream.GetNumberOfBytesUsed(), buf.begin() + 2);
    state.cl->Send(buf.data(), buf.size(), reliable);

    return;
  }

  auto n = strlen(jsonContent);
  std::vector<uint8_t> buf(n + 1);
  buf[0] = Networking::MinPacketId;
  memcpy(buf.data() + 1, jsonContent, n);

  state.cl->Send(buf.data(), buf.size(), reliable);
}
