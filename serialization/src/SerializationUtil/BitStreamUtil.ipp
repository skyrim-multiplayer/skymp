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

template <class T>
T ReadFromBitStream(SLNet::BitStream& stream)
{
  T value;
  ReadFromBitStream(stream, value);
  return value;
}

}
