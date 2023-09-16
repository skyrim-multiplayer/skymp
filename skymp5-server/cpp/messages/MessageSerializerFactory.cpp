#include "MessageSerializerFactory.h"
#include "MinPacketId.h"
#include "MsgType.h"
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>
#include <spdlog/spdlog.h>
#include "Messages.h"

namespace {
template <class Message>
void Serialize(const nlohmann::json& inputJson, SLNet::BitStream& outputStream)
{
  Message message;
  message.ReadJson(
    inputJson); // may throw. we shouldn't pollute outputStream in this case

  outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
  outputStream.Write(static_cast<uint8_t>(Message::kHeaderByte));
  message.WriteBinary(outputStream);
}

template <class Message>
std::optional<DeserializeResult> Deserialize(
  const uint8_t* rawMessageJsonOrBinary, size_t length)
{
  if (length >= 2 && rawMessageJsonOrBinary[1] == Message::kHeaderByte) {
    // BitStream requires non-const ref even though it doesn't modify it
    SLNet::BitStream stream(
      const_cast<unsigned char*>(rawMessageJsonOrBinary) + 2, length - 2,
      /*copyData*/ false);

    Message message;
    message.ReadBinary(stream);

    DeserializeResult result;
    result.msgType = static_cast<MsgType>(Message::kMsgType);
    result.message = std::make_unique<Message>(std::move(message));
    result.format = DeserializeInputFormat::Binary;
    return result;
  }

  // TODO: parse json here as well instead of falling back to PacketParser.cpp

  spdlog::trace("Deserialize - Failure: length={}, rawMessageJsonOrBinary[1]={}, kHeaderByte={}", length, length >= 2 ? rawMessageJsonOrBinary[1] : 0, Message::kHeaderByte);
  return std::nullopt;
}
} // namespace

#define REGISTER_MESSAGE(Message)                                             \
  serializeFns[static_cast<size_t>(Message::kMsgType)] = Serialize<Message>;  \
  deserializeFns[static_cast<size_t>(Message::kHeaderByte)] =                 \
    Deserialize<Message>;

std::shared_ptr<MessageSerializer>
MessageSerializerFactory::CreateMessageSerializer()
{
  constexpr auto kSerializeFnMax = static_cast<size_t>(MsgType::Max);

  // A hack to support MovementMessage, the first message that was ported to
  // binary back in 2021 MovementMessage uses 'M' char instead of MsgType to
  // identify itself in binary encoded packets
  constexpr auto kDeserializeFnMax = std::max(
    static_cast<size_t>(MovementMessage::kHeaderByte) + 1, kSerializeFnMax);

  std::vector<MessageSerializer::SerializeFn> serializeFns(kSerializeFnMax);
  std::vector<MessageSerializer::DeserializeFn> deserializeFns(
    kDeserializeFnMax);

  REGISTER_MESSAGE(MovementMessage)
  REGISTER_MESSAGE(UpdateAnimationMessage)

  // make_shared isn't working for private constructors
  return std::shared_ptr<MessageSerializer>(
    new MessageSerializer(serializeFns, deserializeFns));
}

MessageSerializer::MessageSerializer(
  std::vector<SerializeFn> serializerFns_,
  std::vector<DeserializeFn> deserializerFns_)
  : serializerFns(serializerFns_)
  , deserializerFns(deserializerFns_)
{
}

void MessageSerializer::Serialize(const char* jsonContent,
                                  SLNet::BitStream& outputStream)
{
  // TODO(perf): consider using simdjson for parsing OR even use JsValue
  // directly

  // TODO: logging and write raw instead of throwing exception
  const auto parsedJson = nlohmann::json::parse(jsonContent);

  // TODO: logging and write raw instead of throwing exception
  const auto msgType = static_cast<MsgType>(parsedJson.at("t").get<int>());

  auto index = static_cast<size_t>(msgType);
  if (index >= serializerFns.size()) {
    // TODO: logging
    outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
    outputStream.Write(jsonContent, strlen(jsonContent));
    return;
  }

  auto serializerFn = serializerFns[index];
  if (!serializerFn) {
    // TODO: logging
    outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
    outputStream.Write(jsonContent, strlen(jsonContent));
    return;
  }

  serializerFn(parsedJson, outputStream);
}

std::optional<DeserializeResult> MessageSerializer::Deserialize(
  const uint8_t* rawMessageJsonOrBinary, size_t length)
{
  if (length < 2) {
    spdlog::trace("MessageSerializer::Deserialize - Length < 2");
    return std::nullopt;
  }

  auto headerByte = rawMessageJsonOrBinary[1];
  if (headerByte == '{') {
    return std::nullopt; // Print nothing, should parse JSON as usual
  }

  if (headerByte >= deserializerFns.size()) {
    spdlog::trace("MessageSerializer::Deserialize - {} >= deserializerFns.size() ", static_cast<int>(headerByte));
    return std::nullopt;
  }

  auto deserializerFn = deserializerFns[headerByte];
  if (!deserializerFn) {
    spdlog::trace("MessageSerializer::Deserialize - deserializerFn not found for headerByte {}", static_cast<int>(headerByte));
    return std::nullopt;
  }

  auto result = deserializerFn(rawMessageJsonOrBinary, length);
  if (result == std::nullopt) {
    spdlog::trace("MessageSerializer::Deserialize - deserializerFn returned nullopt for headerByte {}", static_cast<int>(headerByte));
    return std::nullopt;
  }

  return result;
}
