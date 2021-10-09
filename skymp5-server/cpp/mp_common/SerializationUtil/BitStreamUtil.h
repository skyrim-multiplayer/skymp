#include <array>

#include <slikenet/BitStream.h>

namespace SerializationUtil {

#define DECLARE_RAKNET_SAFETY_WRAPPERS(type)                                  \
  inline void WriteToBitStream(SLNet::BitStream& stream, const type& data)    \
  {                                                                           \
    stream.Write(data);                                                       \
  }                                                                           \
  inline void ReadFromBitStream(SLNet::BitStream& stream, type& data)         \
  {                                                                           \
    stream.Read(data);                                                        \
  }

DECLARE_RAKNET_SAFETY_WRAPPERS(uint32_t)
DECLARE_RAKNET_SAFETY_WRAPPERS(float)
DECLARE_RAKNET_SAFETY_WRAPPERS(bool)

template <class T>
void WriteToBitStream(SLNet::BitStream& stream, const std::optional<T>& opt);

template <class T>
void ReadFromBitStream(SLNet::BitStream& stream, std::optional<T>& opt);

template <class T, size_t N>
void WriteToBitStream(SLNet::BitStream& stream, const std::array<T, N>& arr);

template <class T, size_t N>
void ReadFromBitStream(SLNet::BitStream& stream, std::array<T, N>& arr);

template <class T>
T ReadFromBitStream(SLNet::BitStream& stream);

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
  stream.Read(value);
  return value;
}

}
