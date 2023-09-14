#include "UpdateAnimationMessageSerialization.h"

#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>
#include "MsgType.h"
#include "SerializationUtil/BitStreamUtil.h"

void serialization::WriteToBitStream(SLNet::BitStream& stream, const UpdateAnimationMessage& message) 
{
    SerializationUtil::WriteToBitStream(stream, message.idx);
    SerializationUtil::WriteToBitStream(stream, message.numChanges);
    SerializationUtil::WriteToBitStream(stream, message.animEventName);
}

void serialization::ReadFromBitStream(SLNet::BitStream& stream, UpdateAnimationMessage& message)
{
    SerializationUtil::ReadFromBitStream(stream, message.idx);
    SerializationUtil::ReadFromBitStream(stream, message.numChanges);
    SerializationUtil::ReadFromBitStream(stream, message.animEventName);
}

UpdateAnimationMessage serialization::UpdateAnimationMessageFromJson(const nlohmann::json& json)
{
    UpdateAnimationMessage result;
    result.idx = json.at("idx").get<uint32_t>();

    const auto& data = json.at("data");
    result.numChanges = data.at("numChanges");
    result.animEventName = data.at("animEventName");

    return result;
}

nlohmann::json serialization::UpdateAnimationMessageToJson(const UpdateAnimationMessage& message)
{
    auto result = nlohmann::json{
    { "t", MsgType::UpdateAnimation },
    { "idx", message.idx },
    {
      "data",
      {
        { "animEventName", message.animEventName },
        { "numChanges", message.numChanges },
      },
    },
  };
  return result;
}
