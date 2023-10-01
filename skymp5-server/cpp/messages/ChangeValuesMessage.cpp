#include "ChangeValuesMessage.h"
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>
#include "SerializationUtil/BitStreamUtil.h"

void ChangeValuesMessage::WriteBinary(SLNet::BitStream& stream) const
{
  SerializationUtil::WriteToBitStream(stream, idx);
  stream.Write(health);
  stream.Write(magicka);
  stream.Write(stamina);
}

void ChangeValuesMessage::ReadBinary(SLNet::BitStream& stream)
{
  SerializationUtil::ReadFromBitStream(stream, idx);
  stream.Read(health);
  stream.Read(magicka);
  stream.Read(stamina);
}

void ChangeValuesMessage::WriteJson(nlohmann::json& json) const
{
  nlohmann::json res = nlohmann::json::object();
  res["t"] = kMsgType;
  res["data"] = nlohmann::json::object();
  res["data"]["health"] = health;
  res["data"]["magicka"] = magicka;
  res["data"]["stamina"] = stamina;
  if (idx.has_value()) {
    res["idx"] = *idx;
  }
  json = std::move(res);
}

void ChangeValuesMessage::ReadJson(const nlohmann::json& json)
{
  ChangeValuesMessage res;

  auto it = json.find("idx");
  if (it != json.end()) {
    res.idx = it->get<uint32_t>();
  }

  const auto& data = json.at("data");
  res.health = data.at("health").get<float>();
  res.magicka = data.at("magicka").get<float>();
  res.stamina = data.at("stamina").get<float>();

  *this = std::move(res);
}
