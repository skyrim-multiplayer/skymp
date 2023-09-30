#include "TeleportMessage.h"
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

void TeleportMessage::WriteBinary(SLNet::BitStream& stream) const
{
  stream.Write(idx);
  for (int i = 0; i < std::size(pos); ++i) {
    stream.Write(pos[i]);
  }
  for (int i = 0; i < std::size(rot); ++i) {
    stream.Write(rot[i]);
  }
  stream.Write(worldOrCell);
}

void TeleportMessage::ReadBinary(SLNet::BitStream& stream)
{
  stream.Read(idx);
  for (int i = 0; i < std::size(pos); ++i) {
    stream.Read(pos[i]);
  }
  for (int i = 0; i < std::size(rot); ++i) {
    stream.Read(rot[i]);
  }
  stream.Read(worldOrCell);
}

void TeleportMessage::WriteJson(nlohmann::json& json) const
{
  nlohmann::json res = nlohmann::json::object();
  res["t"] = kMsgType;
  res["pos"] = pos;
  res["rot"] = rot;
  res["worldOrCell"] = worldOrCell;
  res["idx"] = idx;
  json = std::move(res);
}

void TeleportMessage::ReadJson(const nlohmann::json& json)
{
  TeleportMessage res;
  res.idx = json.at("idx").get<uint32_t>();

  res.pos = json.at("pos").get<std::array<float,3>>();
  res.rot = json.at("rot").get<std::array<float,3>>();
  res.worldOrCell = json.at("worldOrCell").get<uint32_t>();

  *this = std::move(res);
}
