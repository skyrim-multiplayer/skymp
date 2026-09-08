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

namespace {
void Serialize(const IMessageBase& message, SLNet::BitStream& outputStream)
{
  outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
  message.WriteBinary(outputStream);
}

template <class Message>
void Serialize(const simdjson::dom::element& inputJson,
               SLNet::BitStream& outputStream)
{
  Message message;
  // may throw. we shouldn't pollute outputStream in this case
  message.ReadJson(inputJson);

  Serialize(message, outputStream);
}

// Reused across calls instead of being constructed per message.
//
// simdjson::dom::parser owns its scratch buffers, so constructing one per
// packet means a fresh allocation on every message, and the JSON path used to
// do that once per candidate deserializer. thread_local keeps it safe if
// deserialization ever moves off the main thread.
simdjson::dom::parser& ScratchParser()
{
  thread_local simdjson::dom::parser parser;
  return parser;
}

template <class Message>
std::optional<DeserializeResult> Deserialize(
  const uint8_t* rawMessageJsonOrBinary, size_t length, const simdjson::dom::element* parsedJson)
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

  if (!parsedJson) {
    return std::nullopt;
  }

  Message message;
  message.ReadJson(*parsedJson);

  DeserializeResult result;
  result.msgType = static_cast<MsgType>(Message::kMsgType.value);
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
  const auto len = strlen(jsonContent);

  auto parsedJson = ScratchParser().parse(jsonContent, len);
  if (parsedJson.error()) {
    spdlog::warn("MessageSerializer::Serialize - JSON parse failed, writing "
                 "raw ({} bytes)",
                 len);
    outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
    outputStream.Write(jsonContent, len);
    return;
  }

  auto tResult = parsedJson.get_object().at_key("t").get_uint64();
  if (auto err = tResult.error()) {
    spdlog::warn(
      "MessageSerializer::Serialize - failed to read 't', simdjson "
      "error: {}, writing raw",
      simdjson::error_message(err));
    outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
    outputStream.Write(jsonContent, len);
    return;
  }

  auto index = static_cast<size_t>(tResult.value_unsafe());
  if (index >= serializerFns.size()) {
    spdlog::warn("MessageSerializer::Serialize - type index {} out of range "
                 "(max {}), writing raw",
                 index, serializerFns.size());
    outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
    outputStream.Write(jsonContent, len);
    return;
  }

  auto serializerFn = serializerFns[index];
  if (!serializerFn) {
    spdlog::warn("MessageSerializer::Serialize - no serializer registered for "
                 "type index {}, writing raw",
                 index);
    outputStream.Write(static_cast<uint8_t>(Networking::MinPacketId));
    outputStream.Write(jsonContent, len);
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
    if (spdlog::should_log(spdlog::level::trace)) {
      // Only materialize the message text when trace is actually enabled;
      // this used to allocate a copy of every JSON packet unconditionally.
      spdlog::trace(
        "MessageSerializer::Deserialize - Encountered JSON message {}",
        std::string(reinterpret_cast<const char*>(rawMessageJsonOrBinary) + 1,
                    length - 1));
    }

    // Read the message type once and dispatch straight to its deserializer.
    // Walking the whole table meant every candidate ahead of the real type
    // re-parsed the entire document before rejecting it (addresses the
    // long-standing TODO(#2257) that used to live here).
    thread_local std::string peek;
    peek.assign(reinterpret_cast<const char*>(rawMessageJsonOrBinary) + 1,
                length - 1);

    auto peekResult = ScratchParser().parse(peek);
    if (peekResult.error()) {
      spdlog::trace("MessageSerializer::Deserialize - JSON parse failed");
      return std::nullopt;
    }

    auto typeResult = peekResult.value_unsafe().at_key("t").get_uint64();
    if (typeResult.error()) {
      // Server-produced messages use a string "type" field instead of "t";
      // those are handled further down the pipeline.
      return std::nullopt;
    }

    const auto index = static_cast<size_t>(typeResult.value_unsafe());
    if (index >= deserializerFns.size() || !deserializerFns[index]) {
      spdlog::trace("MessageSerializer::Deserialize - no deserializer for "
                    "JSON message type {}",
                    index);
      return std::nullopt;
    }

    // Materialize into a named local rather than taking the address of an
    // rvalue-qualified accessor's result. dom::element is a lightweight handle
    // into the parser's tape, so this copy is free and removes any doubt about
    // what the pointer outlives.
    const simdjson::dom::element parsedElement = peekResult.value_unsafe();
    return deserializerFns[index](rawMessageJsonOrBinary, length,
                                  &parsedElement);
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
                ""));
    return std::nullopt;
  }

  auto result = deserializerFn(rawMessageJsonOrBinary, length, nullptr);
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
