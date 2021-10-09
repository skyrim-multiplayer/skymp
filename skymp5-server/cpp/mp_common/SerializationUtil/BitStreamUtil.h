#include <array>
#include <optional>

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

}

#include "BitStreamUtil.ipp"
