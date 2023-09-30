#include "UpdateEquipmentMessage.h"

#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

#include "SerializationUtil/BitStreamUtil.h"

void UpdateEquipmentMessage::WriteBinary(SLNet::BitStream& stream) const
{
    stream.Write(idx);
    SerializationUtil::WriteToBitStream(stream, data);
}

void UpdateEquipmentMessage::ReadBinary(SLNet::BitStream& stream)
{
    stream.Read(idx);
    SerializationUtil::ReadFromBitStream(stream, data);
}

void UpdateEquipmentMessage::WriteJson(nlohmann::json& json) const
{
    nlohmann::json res = nlohmann::json::object();
    res["t"] = kMsgType;
    res["idx"] = idx;
    res["data"] = data;
    json = std::move(res);
}

void UpdateEquipmentMessage::ReadJson(const nlohmann::json& json)
{
    UpdateEquipmentMessage res;
    res.idx = json.at("idx").get<uint32_t>();
    res.data = json.at("data");
    *this = std::move(res);
}
