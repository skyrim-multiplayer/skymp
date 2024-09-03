#include "DeathStateContainerMessage.h"
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

#include "SerializationUtil/BitStreamUtil.h"

void DeathStateContainerMessage::WriteBinary(SLNet::BitStream& stream) const
{
  stream.Write(tTeleport.has_value());
  if (tTeleport) {
    tTeleport->WriteBinary(stream);
  }

  stream.Write(tChangeValues.has_value());
  if (tChangeValues) {
    tChangeValues->WriteBinary(stream);
  }

  stream.Write(tIsDead.has_value());
  if (tIsDead) {
    tIsDead->WriteBinary(stream);
  }

  // TODO: use std::optional wrapper like:
  // SerializationUtil::WriteToBitStream(stream, tTeleport);
}

void DeathStateContainerMessage::ReadBinary(SLNet::BitStream& stream)
{
  bool hasTeleport = false;
  stream.Read(hasTeleport);
  if (hasTeleport) {
    tTeleport = TeleportMessage();
    tTeleport->ReadBinary(stream);
  }

  bool hasChangeValues = false;
  stream.Read(hasChangeValues);
  if (hasChangeValues) {
    tChangeValues = ChangeValuesMessage();
    tChangeValues->ReadBinary(stream);
  }

  bool hasIsDead = false;
  stream.Read(hasIsDead);
  if (hasIsDead) {
    tIsDead = UpdatePropertyMessage();
    tIsDead->ReadBinary(stream);
  }
}

void DeathStateContainerMessage::WriteJson(nlohmann::json& json) const
{
  nlohmann::json res = nlohmann::json::object();
  res["t"] = kMsgType;

  if (tTeleport) {
    tTeleport->WriteJson(res["tTeleport"]);
  } else {
    res["tTeleport"] = nlohmann::json{};
  }

  if (tChangeValues) {
    tChangeValues->WriteJson(res["tChangeValues"]);
  } else {
    res["tChangeValues"] = nlohmann::json{};
  }

  if (tIsDead) {
    tIsDead->WriteJson(res["tIsDead"]);
  } else {
    res["tIsDead"] = nlohmann::json{};
  }

  json = std::move(res);
}

void DeathStateContainerMessage::ReadJson(const nlohmann::json& json)
{
  DeathStateContainerMessage res;

  auto it = json.find("tTeleport");
  if (it != json.end() && it->is_object()) {
    res.tTeleport = TeleportMessage();
    res.tTeleport->ReadJson(*it);
  }

  it = json.find("tChangeValues");
  if (it != json.end() && it->is_object()) {
    res.tChangeValues = ChangeValuesMessage();
    res.tChangeValues->ReadJson(*it);
  }

  it = json.find("tIsDead");
  if (it != json.end() && it->is_object()) {
    res.tIsDead = UpdatePropertyMessage();
    res.tIsDead->ReadJson(*it);
  }

  *this = std::move(res);
}
