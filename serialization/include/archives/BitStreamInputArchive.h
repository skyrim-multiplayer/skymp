#pragma once
#include "concepts/Concepts.h"
#include <optional>
#include <slikenet/BitStream.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "BitStreamUtil.h"
#include "BitStreamUtil.ipp"

class BitStreamInputArchive
{
public:
  explicit BitStreamInputArchive(RakNet::BitStream& bitStream)
    : bs(bitStream)
  {
  }

  template <IntegralConstant T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    // Compile time constant. Do nothing
    // Maybe worth adding equality check
    bs.IgnoreBytes(sizeof(T));
    return *this;
  }

  template <StringLike T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    ReadFromBitStream(bs, value);
    return *this;
  }

  // Specialization for std::array
  template <typename T, std::size_t N>
  JsonInputArchive& Serialize(const char* key, std::array<T, N>& value)
  {
    for (size_t i = 0; i < N; ++i) {
      Serialize("element", value[i]);
    }
    return *this;
  }

  template <ContainerLike T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    uint32_t n = 0;
    Serialize("size", n);

    for (size_t i = 0; i < n; ++i) {
      Serialize("element", value[i]);
    }
    return *this;
  }

  template <Optional T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    bool hasValue = false;
    Serialize("hasValue", hasValue);
    if (hasValue) {
      typename T::value_type actualValue;
      Serialize("value", actualValue);
      value = actualValue;
    } else {
      value = std::nullopt;
    }
    return *this;
  }

  template <Arithmetic T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    ReadFromBitStream(bs, value);
    return *this;
  }

  template <NoneOfTheAbove T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    Serialize("child", value);
    return *this;
  }

  Raknet::Bitsrem& bs;
};
