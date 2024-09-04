#pragma once
#include "concepts/Concepts.h"
#include <optional>
#include <slikenet/BitStream.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "../impl/BitStreamUtil.h"
#include "../impl/BitStreamUtil.ipp"

class BitStreamOutputArchive
{
public:
  explicit BitStreamOutputArchive(RakNet::BitStream& bitStream)
    : bs(bitStream)
  {
  }

  template <IntegralConstant T>
  BitStreamOutputArchive& Serialize(const char* key, T& value)
  {
    WriteToBitStream(bs, value);
    return *this;
  }

  template <StringLike T>
  BitStreamOutputArchive& Serialize(const char* key, T& value)
  {
    WriteToBitStream(bs, value);
    return *this;
  }

  // Specialization for std::array
  template <typename T, std::size_t N>
  BitStreamOutputArchive& Serialize(const char* key, std::array<T, N>& value)
  {
    for (size_t i = 0; i < N; ++i) {
      Serialize("element", value[i]);
    }
    return *this;
  }

  template <ContainerLike T>
  BitStreamOutputArchive& Serialize(const char* key, T& value)
  {
    uint32_t n = static_cast<uint32_t>(value.size());
    Serialize("size", n);

    for (size_t i = 0; i < n; ++i) {
      Serialize("element", value[i]);
    }
    return *this;
  }

  template <Optional T>
  BitStreamOutputArchive& Serialize(const char* key, T& value)
  {
    bool hasValue = value.has_value();
    Serialize("hasValue", hasValue);
    if (hasValue) {
      typename T::value_type& actualValue = *value;
      Serialize("value", actualValue);
    }
    return *this;
  }

  template <Arithmetic T>
  BitStreamOutputArchive& Serialize(const char* key, T& value)
  {
    WriteToBitStream(bs, value);
    return *this;
  }

  template <NoneOfTheAbove T>
  BitStreamOutputArchive& Serialize(const char* key, T& value)
  {
    Serialize("child", value);
    return *this;
  }

  Raknet::BitStream& bs;
};
