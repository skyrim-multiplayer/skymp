#include "MessageSerializerFactory.h"
#include "Messages.h"
#include "MinPacketId.h"
#include "MsgType.h"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <slikenet/BitStream.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace {
void Serialize(const IMessageBase& message, SLNet::BitStream& outputStream)
{
  outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
  message.WriteBinary(outputStream);
}

template <class Message>
void Serialize(const simdjson::dom::element& inputJson, SLNet::BitStream& outputStream)
{
  Message message;
  // may throw. we shouldn't pollute outputStream in this case
  message.ReadJson(inputJson);

  Serialize(message, outputStream);
}

template <class Message>
std::optional<DeserializeResult> Deserialize(
  const uint8_t* rawMessageJsonOrBinary, size_t length)
{
  if (length >= 2 && rawMessageJsonOrBinary[1] == Message::kMsgType.value) {
    // byte 0 is packet id => skipping here
    // byte 1 is message type => letting Message::ReadBinary handle it
    // kMsgReadBinaryStart is 1, not 2 because of Message::ReadBinary design
    constexpr auto kMsgReadBinaryOffset = 1;

    // BitStream requires non-const ref even though it doesn't modify it
    SLNet::BitStream stream(
      const_cast<unsigned char*>(rawMessageJsonOrBinary) +
        kMsgReadBinaryOffset,
      length - kMsgReadBinaryOffset,
      /*copyData*/ false);

    Message message;
    message.ReadBinary(stream);

    DeserializeResult result;
    result.msgType = static_cast<MsgType>(Message::kMsgType.value);
    result.message = std::make_unique<Message>(std::move(message));
    result.format = DeserializeInputFormat::Binary;
    return result;
  }

  std::string str(reinterpret_cast<const char*>(rawMessageJsonOrBinary + 1),
                  length - 1);
  simdjson::dom::parser sjParser;
  auto parseResult = sjParser.parse(str);
  if (auto err = parseResult.error()) {
    throw std::runtime_error(fmt::format("failed to parse message, simdjson error: {}", simdjson::error_message(err)));
  }
  auto parsedJson = parseResult.value_unsafe();

  auto msgTypeResult = parsedJson.at_key("t").get_uint64();
  if (msgTypeResult.error() == simdjson::NO_SUCH_FIELD) {
    // Messages produced by the server use string "type" instead of integer "t"
    // We will refactor them out at some point
    return std::nullopt;
  }
  if (auto err = msgTypeResult.error()) {
    throw std::runtime_error(fmt::format("failed to get message type, simdjson error: {}", simdjson::error_message(err)));
  }
  auto msgType = msgTypeResult.value_unsafe();

  if (msgType != Message::kMsgType) {
    // In case of JSON we keep searching in deserializers array
    return std::nullopt;
  }

  Message message;
  message.ReadJson(parsedJson);

  DeserializeResult result;
  result.msgType = static_cast<MsgType>(msgType);
  result.message = std::make_unique<Message>(std::move(message));
  result.format = DeserializeInputFormat::Json;
  return result;
}
} // namespace

#define REGISTER_MESSAGE(Message)                                             \
  serializeFns[static_cast<size_t>(Message::kMsgType)] = Serialize<Message>;  \
  deserializeFns[static_cast<size_t>(Message::kMsgType)] =                    \
    Deserialize<Message>;

std::shared_ptr<MessageSerializer>
MessageSerializerFactory::CreateMessageSerializer()
{
  constexpr auto kSerializeFnMax = static_cast<size_t>(MsgType::Max);
  constexpr auto kDeserializeFnMax = static_cast<size_t>(MsgType::Max);

  std::vector<MessageSerializer::SerializeFn> serializeFns(kSerializeFnMax);
  std::vector<MessageSerializer::DeserializeFn> deserializeFns(
    kDeserializeFnMax);

  REGISTER_MESSAGES

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
  // TODO(#2257): perf: think if JsValue should be used directly

  simdjson::dom::parser sjParser;
  // TODO(#2257): logging and write raw instead of throwing exception
  auto parsedJson = sjParser.parse(jsonContent, strlen(jsonContent));

  // TODO(#2257): logging and write raw instead of throwing exception
  auto tResult = parsedJson.get_object().at_key("t").get_uint64();
  if (auto err = tResult.error()) {
    throw std::runtime_error(fmt::format("failed to read 't' of a message, simdjson error: {}", simdjson::error_message(err)));
  }

  auto index = static_cast<size_t>(tResult.value_unsafe());
  if (index >= serializerFns.size()) {
    // TODO(#2257): logging
    outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
    outputStream.Write(jsonContent, strlen(jsonContent));
    return;
  }

  auto serializerFn = serializerFns[index];
  if (!serializerFn) {
    // TODO(#2257): logging
    outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
    outputStream.Write(jsonContent, strlen(jsonContent));
    return;
  }

  serializerFn(parsedJson.value(), outputStream);
}

void MessageSerializer::Serialize(const IMessageBase& message,
                                  SLNet::BitStream& outputStream)
{
  ::Serialize(message, outputStream);
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
    std::string s(reinterpret_cast<const char*>(rawMessageJsonOrBinary) + 1,
                  length - 1);
    spdlog::trace(
      "MessageSerializer::Deserialize - Encountered JSON message {}", s);
    // TODO(#2257): try to pass JSON in advance, avoid parsing each time
    for (auto fn : deserializerFns) {
      if (fn) {
        auto result = fn(rawMessageJsonOrBinary, length);
        if (result) {
          spdlog::trace("MessageSerializer::Deserialize - Deserialized");
          return result;
        }
      }
    }
    spdlog::trace("MessageSerializer::Deserialize - Failed to deserialize, "
                  "falling back to PacketParser.cpp");
    return std::nullopt;
  }

  if (headerByte >= deserializerFns.size()) {
    spdlog::trace(
      "MessageSerializer::Deserialize - {} >= deserializerFns.size() ",
      static_cast<int>(headerByte));
    return std::nullopt;
  }

  auto deserializerFn = deserializerFns[headerByte];
  if (!deserializerFn) {
    spdlog::warn(
      "MessageSerializer::Deserialize - deserializerFn not found "
      "for headerByte {}, (full message was {})",
      static_cast<int>(headerByte),
      fmt::join(std::vector<uint8_t>(rawMessageJsonOrBinary,
                                     rawMessageJsonOrBinary + length),
                ", "));
    return std::nullopt;
  }

  auto result = deserializerFn(rawMessageJsonOrBinary, length);
  if (result == std::nullopt) {
    spdlog::warn(
      "MessageSerializer::Deserialize - deserializerFn returned "
      "nullopt for headerByte {}, (full message was {})",
      static_cast<int>(headerByte),
      fmt::join(std::vector<uint8_t>(rawMessageJsonOrBinary,
                                     rawMessageJsonOrBinary + length),
                ", "));
    return std::nullopt;
  }

  return result;
}
