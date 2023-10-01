#pragma once

#include "MessageBase.h"
#include "MsgType.h"
#include <cstdint>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <slikenet/types.h>
#include <utility>
#include <vector>

class MessageSerializer;

class MessageSerializerFactory
{
public:
  static std::shared_ptr<MessageSerializer> CreateMessageSerializer();
};

enum class DeserializeInputFormat
{
  Json,
  Binary
};

struct DeserializeResult
{
  MsgType msgType = MsgType::Invalid;
  std::unique_ptr<IMessageBase> message;
  DeserializeInputFormat format = DeserializeInputFormat::Json;
};

class MessageSerializer
{
  friend class MessageSerializerFactory;

public:
  void Serialize(const char* jsonContent, SLNet::BitStream& outputStream);

  void Serialize(const IMessageBase& message, SLNet::BitStream& outputStream);

  std::optional<DeserializeResult> Deserialize(
    const uint8_t* rawMessageJsonOrBinary, size_t length);

private:
  typedef void (*SerializeFn)(const nlohmann::json& inputJson,
                              SLNet::BitStream& outputStream);
  typedef std::optional<DeserializeResult> (*DeserializeFn)(
    const uint8_t* rawMessageJsonOrBinary, size_t length);

  MessageSerializer(std::vector<SerializeFn> serializerFns,
                    std::vector<DeserializeFn> deserializerFns);

  const std::vector<SerializeFn> serializerFns;
  const std::vector<DeserializeFn> deserializerFns;
};
