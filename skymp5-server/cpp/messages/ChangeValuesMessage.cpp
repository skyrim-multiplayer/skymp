#include "ChangeValuesMessage.h"
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

void ChangeValuesMessage::WriteBinary(SLNet::BitStream& stream) const
{
  stream.Write(idx);
  stream.Write(health);
  stream.Write(magicka);
  stream.Write(stamina);
}

void ChangeValuesMessage::ReadBinary(SLNet::BitStream& stream)
{
  stream.Read(idx);
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
  res["idx"] = idx;
  json = std::move(res);
}

void ChangeValuesMessage::ReadJson(const nlohmann::json& json)
{
  ChangeValuesMessage res;
  res.idx = json.at("idx").get<uint32_t>();

  const auto& data = json.at("data");
  res.health = data.at("health").get<float>();
  res.magicka = data.at("magicka").get<float>();
  res.stamina = data.at("stamina").get<float>();

  *this = std::move(res);
}
