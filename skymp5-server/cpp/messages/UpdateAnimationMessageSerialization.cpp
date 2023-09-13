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
    
}

nlohmann::json serialization::UpdateAnimationMessageToJson(const UpdateAnimationMessage& message)
{

}
