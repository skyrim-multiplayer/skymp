#include <array>
#include <optional>

#include <slikenet/BitStream.h>

namespace SerializationUtil {

template <class T>
void WriteToBitStream(SLNet::BitStream& stream, const std::optional<T>& opt)
{
  if (opt) {
    WriteToBitStream(stream, true);
    WriteToBitStream(stream, *opt);
  } else {
    WriteToBitStream(stream, false);
  }
}

template <class T>
void ReadFromBitStream(SLNet::BitStream& stream, std::optional<T>& opt)
{
  if (ReadFromBitStream<bool>(stream)) {
    ReadFromBitStream(stream, opt.emplace());
  } else {
    opt = std::nullopt;
  }
}

template <class T, size_t N>
void WriteToBitStream(SLNet::BitStream& stream, const std::array<T, N>& arr)
{
  for (size_t i = 0; i < N; ++i) {
    stream.Write(arr[i]);
  }
}

template <class T, size_t N>
void ReadFromBitStream(SLNet::BitStream& stream, std::array<T, N>& arr)
{
  for (size_t i = 0; i < N; ++i) {
    stream.Read(arr[i]);
  }
}

void WriteToBitStream(SLNet::BitStream& stream, const std::string& str)
{
  WriteToBitStream(stream, static_cast<uint32_t>(str.size()));
  // TODO: write whole memory at once
  for (char ch : str) {
    WriteToBitStream(stream, static_cast<uint8_t>(ch));
  }
}

void ReadFromBitStream(SLNet::BitStream& stream, std::string& str)
{
  str.clear();

  uint32_t size = 0;
  if (stream.Read(size)) {
    str.reserve(size);
    // TODO: read all memory at once
    for (uint32_t i = 0; i < size; ++i) {
      uint8_t ch = 0;
      ReadFromBitStream(stream, ch);
      str.push_back(ch);
    }
  }
}

template <class T>
T ReadFromBitStream(SLNet::BitStream& stream)
{
  T value;
  ReadFromBitStream(stream, value);
  return value;
}

}
