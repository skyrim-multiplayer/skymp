#pragma once
#include "concepts/Concepts.h"
#include <algorithm>
#include <optional>
#include <slikenet/BitStream.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "../impl/BitStreamUtil.h"
#include "../impl/BitStreamUtil.ipp"

#include <spdlog/spdlog.h>

class BitStreamOutputArchive
{
public:
  explicit BitStreamOutputArchive(SLNet::BitStream& bitStream)
    : bs(bitStream)
  {
  }

  template <IntegralConstant T>
  BitStreamOutputArchive& Serialize(const char* key, T& value)
  {
    using ValueType = typename T::value_type;
    auto actualValue = static_cast<ValueType>(value);
    SerializationUtil::WriteToBitStream(bs, actualValue);
    // spdlog::info("!!! serialized integral constant {} {}", key,
    //              static_cast<int>(static_cast<char>(value)));
    return *this;
  }

  template <StringLike T>
  BitStreamOutputArchive& Serialize(const char* key, T& value)
  {
    uint32_t n = static_cast<uint32_t>(value.size());
    Serialize("size", n);

    for (size_t i = 0; i < n; ++i) {
      Serialize("element", value[i]);
    }
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
    SerializationUtil::WriteToBitStream(bs, value);
    // spdlog::info("!!! serialized arithmetic {} {}", key, value);
    return *this;
  }

  template <typename... Types>
  BitStreamOutputArchive& Serialize(const char* key,
                                    std::variant<Types...>& value)
  {
    uint32_t typeIndex = static_cast<uint32_t>(value.index());

    Serialize("typeIndex", typeIndex);

    auto serializeVisitor = [&](auto& v) { Serialize("value", v); };

    std::visit(serializeVisitor, value);

    return *this;
  }

  template <NoneOfTheAbove T>
  BitStreamOutputArchive& Serialize(const char* key, T& value)
  {
    value.Serialize(*this);
    // spdlog::info("!!! serialized none of the above {}");
    return *this;
  }

  SLNet::BitStream& bs;
};
