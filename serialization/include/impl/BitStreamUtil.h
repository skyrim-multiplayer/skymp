#pragma once
#include <array>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <slikenet/BitStream.h>
#include <string>

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

DECLARE_RAKNET_SAFETY_WRAPPERS(uint64_t)
DECLARE_RAKNET_SAFETY_WRAPPERS(uint32_t)
DECLARE_RAKNET_SAFETY_WRAPPERS(uint16_t)
DECLARE_RAKNET_SAFETY_WRAPPERS(uint8_t)
DECLARE_RAKNET_SAFETY_WRAPPERS(int64_t)
DECLARE_RAKNET_SAFETY_WRAPPERS(int32_t)
DECLARE_RAKNET_SAFETY_WRAPPERS(int16_t)
DECLARE_RAKNET_SAFETY_WRAPPERS(int8_t)
DECLARE_RAKNET_SAFETY_WRAPPERS(float)
DECLARE_RAKNET_SAFETY_WRAPPERS(double)
DECLARE_RAKNET_SAFETY_WRAPPERS(bool)
DECLARE_RAKNET_SAFETY_WRAPPERS(char)

#undef DECLARE_RAKNET_SAFETY_WRAPPERS

template <class T>
void WriteToBitStream(SLNet::BitStream& stream, const std::optional<T>& opt);

template <class T>
void ReadFromBitStream(SLNet::BitStream& stream, std::optional<T>& opt);

template <class T, size_t N>
void WriteToBitStream(SLNet::BitStream& stream, const std::array<T, N>& arr);

template <class T, size_t N>
void ReadFromBitStream(SLNet::BitStream& stream, std::array<T, N>& arr);

void WriteToBitStream(SLNet::BitStream& stream, const std::string& str);

void ReadFromBitStream(SLNet::BitStream& stream, std::string& str);

void WriteToBitStream(SLNet::BitStream& stream, const nlohmann::json& json);

void ReadFromBitStream(SLNet::BitStream& stream, nlohmann::json& json);

template <class T>
T ReadFromBitStream(SLNet::BitStream& stream);

}

#include "BitStreamUtil.ipp"
