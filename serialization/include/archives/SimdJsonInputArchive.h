#pragma once

#include <exception>
#include <fmt/format.h>
#include <simdjson.h>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "concepts/Concepts.h"

class SimdJsonInputArchive
{
public:
  explicit SimdJsonInputArchive(const simdjson::dom::element& j)
    : input(j)
  {
  }

  template <IntegralConstant T>
  SimdJsonInputArchive& Serialize(const char* key, T& output)
  {
    // Compile time constant. Do nothing
    // Maybe worth adding equality check
    return *this;
  }

  template <StringLike T>
  SimdJsonInputArchive& Serialize(const T& output)
  {
    const auto& strResult = input.get_string();
    if (const auto err = strResult.error()) {
      throw std::runtime_error(fmt::format("simdjson: {}", simdjson::error_message(err)));
    }
    if constexpr (std::is_same_v<T, std::string>) {
      output = std::string(strResult.value_unsafe());
    } else {
      static_assert(false, "can only parse to std::string");
    }
    return *this;
  }

  template <typename T, std::size_t N>
  SimdJsonInputArchive& Serialize(std::array<T, N>& output)
  {
    const auto& arrayResult = input.get_array();
    if (const auto err = arrayResult.error()) {
      throw std::runtime_error(fmt::format("simdjson: {}", simdjson::error_message(err)));
    }
    const auto& input = arrayResult.value_unsafe();

    size_t idx = 0;
    for (const auto& inputItem : input) {
      if (idx >= N) {
        throw std::runtime_error(fmt::format("index {} out of bounds for output", idx));
      }

      try {
        SimdJsonInputArchive itemArchive(inputItem);
        itemArchive.Serialize(output[idx]);
      } catch (const std::exception& e) {
        throw std::runtime_error(fmt::format("couldn't get array index {}: {}", idx, e.what()));
      }

      ++idx;
    }
    if (idx != N) {
      throw std::runtime_error(fmt::format("index {} out of bounds for input", idx));
    }

    return *this;
  }

  template <ContainerLike T>
  SimdJsonInputArchive& Serialize(T& output)
  {
    output.clear();

    const auto& arrayResult = input.get_array();
    if (const auto err = arrayResult.error()) {
      throw std::runtime_error(fmt::format("simdjson: {}", simdjson::error_message(err)));
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
        throw std::runtime_error(fmt::format("couldn't get array index {}: {}", idx, e.what()));
      }

      ++idx;
    }

    return *this;
  }

  template <Arithmetic T>
  SimdJsonInputArchive& Serialize(T& output)
  {
    if constexpr (std::is_same_v<T, float>) {
      double tmp;
      Serialize(tmp);
      output = tmp;
      return *this;
    } else if constexpr (std::is_same_v<T, unsigned int> || std::is_same_v<T, unsigned char>) {
      uint64_t tmp;
      Serialize(tmp);
      output = tmp;
      return *this;
    } else {
    const auto err = input.get(output);
    if (err) {
      throw std::runtime_error(fmt::format("simdjson: {}", simdjson::error_message(err)));
    }
    return *this;
    }
  }

  template <NoneOfTheAbove T>
  SimdJsonInputArchive& Serialize(T& output)
  {
    try {
      output.Serialize(*this);
    } catch (const std::exception& e) {
      throw std::runtime_error(fmt::format("failed to call Serialize from SimdJsonInputArchive: {}", e.what()));
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
      throw std::runtime_error(fmt::format("simdjson failed to get key '{}': {}", key, simdjson::error_message(err)));
    }

    try {
      T outputItem;
      SimdJsonInputArchive itemArchive(result.value_unsafe());
      itemArchive.Serialize(outputItem);
      output.emplace(outputItem);
    } catch (const std::exception& e) {
      throw std::runtime_error(fmt::format("failed to get key '{}': {}", key, e.what()));
    }

    return *this;
  }

  template <class T>
  SimdJsonInputArchive& Serialize(const char* key, T& output)
  {
    const auto& result = input.at_key(key);
    if (const auto err = result.error()) {
      throw std::runtime_error(fmt::format("simdjson failed to get key '{}': {}", key, simdjson::error_message(err)));
    }
    try {
      SimdJsonInputArchive itemArchive(result.value_unsafe());
      itemArchive.Serialize(output);
    } catch (const std::exception& e) {
      throw std::runtime_error(fmt::format("failed to get key '{}': {}", key, e.what()));
    }
    return *this;
  }

private:
  const simdjson::dom::element& input;
};
