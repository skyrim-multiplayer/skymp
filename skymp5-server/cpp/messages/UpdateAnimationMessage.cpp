#include "UpdateAnimationMessage.h"
#include "SerializationUtil/BitStreamUtil.h"
#include <nlohmann/json.hpp>

void UpdateAnimationMessage::WriteBinary(SLNet::BitStream& stream) const
{
    SerializationUtil::WriteToBitStream(stream, idx);
    SerializationUtil::WriteToBitStream(stream, numChanges);
    SerializationUtil::WriteToBitStream(stream, animEventName);
}

void UpdateAnimationMessage::ReadBinary(SLNet::BitStream& stream)
{
    SerializationUtil::ReadFromBitStream(stream, idx);
    SerializationUtil::ReadFromBitStream(stream, numChanges);
    SerializationUtil::ReadFromBitStream(stream, animEventName);
}

void UpdateAnimationMessage::WriteJson(nlohmann::json &json) const
{
    auto result = nlohmann::json{
    { "t", MsgType::UpdateAnimation },
    { "idx", idx },
    {
      "data",
      {
        { "animEventName", animEventName },
        { "numChanges", numChanges },
      },
    },
  };
  json = std::move(result);
}

void UpdateAnimationMessage::ReadJson(const nlohmann::json &json)
{
    UpdateAnimationMessage result;
    result.idx = json.at("idx").get<uint32_t>();

    const auto& data = json.at("data");
    result.numChanges = data.at("numChanges");
    result.animEventName = data.at("animEventName");
}
