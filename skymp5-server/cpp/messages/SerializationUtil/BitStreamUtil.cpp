#include "BitStreamUtil.h"
#include <nlohmann/json.hpp>

void SerializationUtil::WriteToBitStream(SLNet::BitStream& stream,
                                         const std::string& str)
{
  WriteToBitStream(stream, static_cast<uint32_t>(str.size()));
  stream.Write(str.data(), static_cast<unsigned int>(str.size()));
}

void SerializationUtil::ReadFromBitStream(SLNet::BitStream& stream,
                                          std::string& str)
{
  uint32_t size = 0;
  if (stream.Read(size)) {
    std::string tmp;
    tmp.resize(size);
    stream.Read(tmp.data(), tmp.size());
    str = std::move(tmp);
  } else {
    str.clear();
  }
}

void SerializationUtil::WriteToBitStream(SLNet::BitStream& stream,
                                         const nlohmann::json& json)
{
  WriteToBitStream(stream, json.dump());
}

void SerializationUtil::ReadFromBitStream(SLNet::BitStream& stream,
                                          nlohmann::json& json)
{
  std::string s;
  ReadFromBitStream(stream, s);
  try {
    json = nlohmann::json::parse(s);
  } catch (nlohmann::json::parse_error& e) {
    json = nlohmann::json{};
  }
}
