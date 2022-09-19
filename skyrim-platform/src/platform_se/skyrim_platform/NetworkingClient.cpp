#ifdef _SP_WITH_NETWORKING_CLIENT

#  include "NetworkingClient.h"

#  include <vector>

#  include "MovementMessage.h"
#  include "MovementMessageSerialization.h"
#  include "MsgType.h"

#  include <nlohmann/json.hpp>

NetworkingClient::State& GetState()
{
  static NetworkingClient::State state;
  return state;
}

void NetworkingClient::Create(const char* targetHostname, uint16_t targetPort)
{
  GetState().client = Networking::CreateClient(targetHostname, targetPort);
}

void NetworkingClient::Destroy()
{
  GetState().client.reset();
}

bool NetworkingClient::IsConnected()
{
  auto state = GetState();
  return state.client && state.client->IsConnected();
}

void NetworkingClient::Tick(OnPacket onPacket, void* state_)
{
  auto state = GetState();
  if (!state.client)
    return;

  std::pair<OnPacket, void*> packetAndState(onPacket, state_);

  state.client->Tick(
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

void NetworkingClient::Send(const char* jsonContent, bool reliable)
{
  auto state = GetState();
  if (!state.client) {
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
    state.client->Send(buf.data(), buf.size(), reliable);

    return;
  }

  auto n = strlen(jsonContent);
  std::vector<uint8_t> buf(n + 1);
  buf[0] = Networking::MinPacketId;
  memcpy(buf.data() + 1, jsonContent, n);

  state.client->Send(buf.data(), buf.size(), reliable);
}
#endif
