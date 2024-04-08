#include "UpdatePropertyMessage.h"

#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

#include "SerializationUtil/BitStreamUtil.h"

void UpdatePropertyMessage::WriteBinary(SLNet::BitStream& stream) const
{
  stream.Write(idx);
  SerializationUtil::WriteToBitStream(stream, propName);
  stream.Write(refrId);
  SerializationUtil::WriteToBitStream(stream, data);
  SerializationUtil::WriteToBitStream(stream, baseRecordType);
}

void UpdatePropertyMessage::ReadBinary(SLNet::BitStream& stream)
{
  stream.Read(idx);
  SerializationUtil::ReadFromBitStream(stream, propName);
  stream.Read(refrId);
  SerializationUtil::ReadFromBitStream(stream, data);
  SerializationUtil::ReadFromBitStream(stream, baseRecordType);
}

void UpdatePropertyMessage::WriteJson(nlohmann::json& json) const
{
  nlohmann::json res = nlohmann::json::object();
  res["t"] = kMsgType;
  res["idx"] = idx;
  res["propName"] = propName;
  res["refrId"] = refrId;
  res["data"] = data;
  res["baseRecordType"] =
    baseRecordType ? nlohmann::json(*baseRecordType) : nlohmann::json{};
  json = std::move(res);
}

void UpdatePropertyMessage::ReadJson(const nlohmann::json& json)
{
  UpdatePropertyMessage res;
  res.idx = json.at("idx").get<uint32_t>();
  res.propName = json.at("propName").get<std::string>();
  res.refrId = json.at("refrId").get<uint32_t>();
  res.data = json.at("data");

  auto it = json.find("baseRecordType");
  if (it != json.end()) {
    res.baseRecordType = it->get<std::string>();
  }
  *this = std::move(res);
}
