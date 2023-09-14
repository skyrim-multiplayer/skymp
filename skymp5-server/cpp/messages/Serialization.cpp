#include "Serialization.h"
#include "NetworkingInterface.h"
#include "MsgType.h"
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

#include "MovementMessageSerialization.h"
#include "UpdateAnimationMessageSerialization.h"

namespace {
// TODO(perf): make outputBuffer a BitStream and write directly instead of copying
template <class Message>
void SerializeMessageBinary(const Message &message, std::vector<uint8_t> &outputBuffer)
{
    SLNet::BitStream stream;
    serialization::WriteToBitStream(stream, message);

    outputBuffer.resize(stream.GetNumberOfBytesUsed() + 2);
    outputBuffer[0] = Networking::MinPacketId;
    outputBuffer[1] = Message::kHeaderByte;
    std::copy(stream.GetData(), stream.GetData() + stream.GetNumberOfBytesUsed(), outputBuffer.begin() + 2);
}
}

void Serialization::SerializeMessage(const char *jsonContent, std::vector<uint8_t> &outputBuffer)
{
    // TODO(perf): consider using simdjson for parsing OR even use JsValue directly
    const auto parsedJson = nlohmann::json::parse(jsonContent);
    const auto msgType = static_cast<MsgType>(parsedJson.at("t").get<int>());

    if (msgType == MsgType::UpdateMovement) {
        return SerializeMessageBinary<MovementMessage>(serialization::MovementMessageFromJson(parsedJson), outputBuffer);
    }
    if (msgType == MsgType::UpdateAnimation) {
        return SerializeMessageBinary<UpdateAnimationMessage>(serialization::UpdateAnimationMessageFromJson(parsedJson), outputBuffer);
    }

    auto n = strlen(jsonContent);
    outputBuffer.resize(n + 1);
    outputBuffer[0] = Networking::MinPacketId;
    memcpy(outputBuffer.data() + 1, jsonContent, n);
}
