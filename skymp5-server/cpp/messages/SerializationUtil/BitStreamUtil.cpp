#include "BitStreamUtil.h"

void SerializationUtil::WriteToBitStream(SLNet::BitStream& stream, const std::string& str)
{
  WriteToBitStream(stream, static_cast<uint32_t>(str.size()));
  stream.Write(str.data(), static_cast<unsigned int>(str.size()));
}

void SerializationUtil::ReadFromBitStream(SLNet::BitStream& stream, std::string& str)
{
  uint32_t size = 0;
  if (stream.Read(size)) {
    std::string tmp;
    tmp.resize(size);
    stream.Read(tmp.data(), tmp.size());
    str = std::move(tmp);
  }
  else {
    str.clear();
  }
}
