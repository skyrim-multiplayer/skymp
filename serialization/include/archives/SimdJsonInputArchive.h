#pragma once

#include <concepts>
#include <cstdint>
#include <exception>
#include <fmt/format.h>
#include <limits>
#include <optional>
#include <simdjson.h>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "concepts/Concepts.h"

namespace {

struct SimdJsonSupportedTypeAdapter
{
  template <std::signed_integral T>
  int64_t MapType(T);

  template <std::unsigned_integral T>
  uint64_t MapType(T);

  template <std::floating_point T>
  double MapType(T);

  bool MapType(bool);
};

template <class T>
using SimdJsonSupportedType =
  decltype(std::declval<SimdJsonSupportedTypeAdapter>().MapType(
    std::declval<T>()));

template <class T>
concept ArithmeticSimdjsonUnsupported = Arithmetic<T> && !
std::is_same_v<T, SimdJsonSupportedType<T>>;

template <class T>
concept ArithmeticSimdjsonSupported =
  Arithmetic<T> && std::is_same_v<T, SimdJsonSupportedType<T>>;

}

class SimdJsonInputArchive
{
public:
  explicit SimdJsonInputArchive(const simdjson::dom::element& j)
    : input(j)
  {
  }

  // TODO(#2250): it's probably useless and should be removed
  template <IntegralConstant T>
  SimdJsonInputArchive& Serialize(const char* key, T& output)
  {
    // Compile time constant. Do nothing
    // Maybe worth adding equality check
    return *this;
  }

  template <StringLike T>
  SimdJsonInputArchive& Serialize(T& output)
  {
    static_assert(!sizeof(T), "can only parse to std::string");
  }

  SimdJsonInputArchive& Serialize(std::string& output)
  {
    const auto& strResult = input.get_string();
    if (const auto err = strResult.error()) {
      throw std::runtime_error(fmt::format(
        "simdjson (type in:{} out:string): {}",
        static_cast<char>(input.type()), simdjson::error_message(err)));
    }
    output = std::string(strResult.value_unsafe());
    return *this;
  }

  template <typename T, std::size_t N>
  SimdJsonInputArchive& Serialize(std::array<T, N>& output)
  {
    const auto& arrayResult = input.get_array();
    if (const auto err = arrayResult.error()) {
      throw std::runtime_error(
        fmt::format("simdjson (type in:{} out:array<{}>): {}",
                    static_cast<char>(input.type()), typeid(T).name(),
                    simdjson::error_message(err)));
    }
    const auto& input = arrayResult.value_unsafe();

    size_t idx = 0;
    for (const auto& inputItem : input) {
      if (idx >= N) {
        //           v
        // I: ............)
        // O: .......)
        throw std::runtime_error(fmt::format(
          "index {} out of bounds for output (input is bigger)", idx));
      }

      try {
        SimdJsonInputArchive itemArchive(inputItem);
        itemArchive.Serialize(output[idx]);
      } catch (const std::exception& e) {
        throw std::runtime_error(
          fmt::format("couldn't get array index {}: {}", idx, e.what()));
      }

      ++idx;
    }
    if (idx != N) {
      //         v
      // I: .....)
      // O: ............)
      throw std::runtime_error(fmt::format(
        "index {} out of bounds for input ({} elements expected)", idx, N));
    }

    return *this;
  }

  template <ContainerLike T>
  SimdJsonInputArchive& Serialize(T& output)
  {
    output.clear();

    const auto& arrayResult = input.get_array();
    if (const auto err = arrayResult.error()) {
      throw std::runtime_error(fmt::format(
        "simdjson (type in:{} out:{}): {}", static_cast<char>(input.type()),
        typeid(T).name(), simdjson::error_message(err)));
    }
    const auto& input = arrayResult.value_unsafe();

    size_t idx = 0;
    for (const auto& inputItem : input) {
      try {
        typename T::value_type outputItem;
        SimdJsonInputArchive itemArchive(inputItem);
        itemArchive.Serialize(outputItem);
        output.emplace_back(std::move(outputItem));
      } catch (const std::exception& e) {
        throw std::runtime_error(
          fmt::format("couldn't get array index {}: {}", idx, e.what()));
      }

      ++idx;
    }

    return *this;
  }

  template <ArithmeticSimdjsonSupported T>
  SimdJsonInputArchive& Serialize(T& output)
  {
    const auto err = input.get(output);
    if (err) {
      throw std::runtime_error(fmt::format(
        "simdjson (type in:{} out:{}): {}", static_cast<char>(input.type()),
        typeid(T).name(), simdjson::error_message(err)));
    }
    return *this;
  }

  template <ArithmeticSimdjsonUnsupported T>
  SimdJsonInputArchive& Serialize(T& output)
  {
    try {
      SimdJsonSupportedType<T> tmp;
      Serialize(tmp);
      if (std::is_integral_v<T> &&
          !(std::numeric_limits<T>::min() <= tmp &&
            tmp <= std::numeric_limits<T>::max())) {
        throw std::runtime_error(
          fmt::format("value {} doesn't fit into the requested type {}", tmp,
                      typeid(T).name()));
      }
      output = std::move(tmp);
    } catch (const std::exception& e) {
      throw std::runtime_error(fmt::format(
        "failed to call Serialize (type in:{} out:adapter {} -> {}): {}",
        static_cast<char>(input.type()), typeid(T).name(),
        typeid(SimdJsonSupportedType<T>).name(), e.what()));
    }
    return *this;
  }

  // This function is called when for non-trivial types.
  // It's expected that a special function will handle this, implemented by the
  // said type
  template <NoneOfTheAbove T>
  SimdJsonInputArchive& Serialize(T& output)
  {
    try {
      output.Serialize(*this);
    } catch (const std::exception& e) {
      throw std::runtime_error(
        fmt::format("failed to call custom Serialize for type {}: {}",
                    typeid(T).name(), e.what()));
    }
    return *this;
  }

  template <class T>
  SimdJsonInputArchive& Serialize(const char* key, std::optional<T>& output)
  {
    const auto& result = input.at_key(key);
    const auto err = result.error();
    if (err == simdjson::NO_SUCH_FIELD) {
      output.reset();
      return *this;
    }
    if (err) {
      throw std::runtime_error(
        fmt::format("simdjson failed to get key '{}': {}", key,
                    simdjson::error_message(err)));
    }

    try {
      T outputItem;
      SimdJsonInputArchive itemArchive(result.value_unsafe());
      itemArchive.Serialize(outputItem);
      output.emplace(outputItem);
    } catch (const std::exception& e) {
      throw std::runtime_error(
        fmt::format("failed to get key '{}': {}", key, e.what()));
    }

    return *this;
  }

  template <class T>
  SimdJsonInputArchive& Serialize(const char* key, T& output)
  {
    const auto& result = input.at_key(key);
    if (const auto err = result.error()) {
      throw std::runtime_error(
        fmt::format("simdjson failed to get key '{}': {}", key,
                    simdjson::error_message(err)));
    }
    try {
      SimdJsonInputArchive itemArchive(result.value_unsafe());
      itemArchive.Serialize(output);
    } catch (const std::exception& e) {
      throw std::runtime_error(
        fmt::format("failed to get key '{}': {}", key, e.what()));
    }
    return *this;
  }

private:
  const simdjson::dom::element& input;
};
