#include "MpClientPlugin.h"

#include "FileUtils.h"
#include "MovementMessage.h"
#include "MovementMessageSerialization.h"
#include "MsgType.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <vector>

void MpClientPlugin::CreateClient(State& state, const char* targetHostname,
                                  uint16_t targetPort)
{
  std::string password = kNetworkingPassword;
  // Keep in sync with installer code
  static const std::string kPasswordPath =
    "Data/Platform/Distribution/password";
  static const int kTimeoutMs = 4000;
  try {
    password = Viet::ReadFileIntoString(kPasswordPath);
  } catch (std::exception& e) {
    spdlog::warn("Unable to read password from '{}', will use standard '{}'",
                 kPasswordPath, kNetworkingPassword);
  }
  state.cl = Networking::CreateClient(targetHostname, targetPort, kTimeoutMs,
                                      password.data());
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
    [](void* rawState, Networking::PacketType packetType,
       Networking::PacketData data, size_t length, const char* error) {
      const auto& [onPacket, state] =
        *reinterpret_cast<std::pair<OnPacket, void*>*>(rawState);

      std::string jsonContent;

      if (packetType == Networking::PacketType::Message && length > 1) {
        if (data[1] == MovementMessage::kHeaderByte) {
          MovementMessage movData;
          // BitStream requires non-const ref even though it doesn't modify it
          SLNet::BitStream stream(const_cast<unsigned char*>(data) + 2,
                                  length - 2, /*copyData*/ false);
          serialization::ReadFromBitStream(stream, movData);
          jsonContent = serialization::MovementMessageToJson(movData).dump();
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
    // TODO(#263): we probably should log something here
    return;
  }

  const auto parsedJson = nlohmann::json::parse(jsonContent);
  if (static_cast<MsgType>(parsedJson.at("t").get<int>()) ==
      MsgType::UpdateMovement) {
    const auto movData = serialization::MovementMessageFromJson(parsedJson);
    SLNet::BitStream stream;
    serialization::WriteToBitStream(stream, movData);

    std::vector<uint8_t> buf(stream.GetNumberOfBytesUsed() + 2);
    buf[0] = Networking::MinPacketId;
    buf[1] = MovementMessage::kHeaderByte;
    std::copy(stream.GetData(),
              stream.GetData() + stream.GetNumberOfBytesUsed(),
              buf.begin() + 2);
    state.cl->Send(buf.data(), buf.size(), reliable);

    return;
  }

  auto n = strlen(jsonContent);
  std::vector<uint8_t> buf(n + 1);
  buf[0] = Networking::MinPacketId;
  memcpy(buf.data() + 1, jsonContent, n);

  state.cl->Send(buf.data(), buf.size(), reliable);
}
