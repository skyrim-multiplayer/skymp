#include "OpenContainerMessage.h"
#include <nlohmann/json.hpp>
#include <slikenet/BitStream.h>

void OpenContainerMessage::WriteBinary(SLNet::BitStream& stream) const
{
  stream.Write(target);
}

void OpenContainerMessage::ReadBinary(SLNet::BitStream& stream)
{
  stream.Read(target);
}

void OpenContainerMessage::WriteJson(nlohmann::json& json) const
{
  nlohmann::json res = nlohmann::json::object();
  res["t"] = kMsgType;
  res["target"] = target;
  json = std::move(res);
}

void OpenContainerMessage::ReadJson(const nlohmann::json& json)
{
  OpenContainerMessage res;
  res.target = json.at("target").get<uint32_t>();
  *this = std::move(res);
}
