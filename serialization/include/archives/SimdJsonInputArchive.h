#pragma once

#include <exception>
#include <fmt/format.h>
#include <simdjson.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "concepts/Concepts.h"

class SimdJsonInputArchive
{
public:
  explicit SimdJsonInputArchive(const simdjson::dom::element& j)
    : input(j)
  {
  }

  template <IntegralConstant T>
  SimdJsonInputArchive& Serialize(const char* key, T& value)
  {
    // Compile time constant. Do nothing
    // Maybe worth adding equality check
    return *this;
  }

  template <StringLike T>
  SimdJsonInputArchive& Serialize(const char* key, T& output)
  {
    auto err = input.at_key(key).get(output);
    if (err) {
      throw std::runtime_error(fmt::format("simdjson error {}, key='{}'", simdjson::error_message(err), key));
    }
    return *this;
  }

  template <typename T, std::size_t N>
  SimdJsonInputArchive& Serialize(const char* key, std::array<T, N>& output)
  {
    const auto& arrayResult = input.at_key(key).get_array();
    if (const auto err = arrayResult.error()) {
      throw std::runtime_error(fmt::format("simdjson error {}, key='{}'", simdjson::error_message(err), key));
    }
    const auto& input = arrayResult.value_unsafe();

    size_t idx = 0;
    for (const auto& inputItem : input) {
      if (idx >= N) {
        throw std::runtime_error(fmt::format("simdjson archive: index {} out of bounds for output, key='{}'", idx, key));
      }

      const auto err = inputItem.get(output[idx]);
      if (err) {
        throw std::runtime_error(fmt::format("simdjson error {}, key='{}', index={}", simdjson::error_message(err), key, idx));
      }

      ++idx;
    }
    if (idx != N) {
      throw std::runtime_error(fmt::format("simdjson archive: index {} out of bounds for input, key='{}'", idx, key));
    }

    return *this;
  }

  template <ContainerLike T>
  SimdJsonInputArchive& Serialize(const char* key, T& output)
  {
    output.clear();

    const auto& arrayResult = input.at_key(key).get_array();
    if (const auto err = arrayResult.error()) {
      throw std::runtime_error(fmt::format("simdjson error {}, key='{}'", simdjson::error_message(err), key));
    }
    const auto& input = arrayResult.value_unsafe();

    size_t idx = 0;
    for (const auto& inputItem : input) {
      T outputItem;
      const auto err = inputItem.get(outputItem);
      if (err) {
        throw std::runtime_error(fmt::format("simdjson error {}, key='{}', index={}", simdjson::error_message(err), key, idx));
      }
      output.emplace_back(std::move(outputItem));

      ++idx;
    }

    return *this;
  }

  template <Optional T>
  SimdJsonInputArchive& Serialize(const char* key, T& output)
  {
    typename T::value_type outputItem;
    const simdjson::error_code err = input.at_key(key).get(outputItem); // XXX: should recurse
    if (err == simdjson::NO_SUCH_FIELD) {
      output.reset();
      return *this;
    }
    if (err) {
      throw std::runtime_error(fmt::format("simdjson error {}, key='{}'", simdjson::error_message(err), key));
    }
    output.emplace(std::move(outputItem));
    return *this;
  }

  template <Arithmetic T>
  SimdJsonInputArchive& Serialize(const char* key, T& value)
  {
    // value = j_.at(key).get<T>();
    // j_.at_pointer()
    const auto err = input.at_key(key).get(value);
    if (err) {
      throw std::runtime_error(fmt::format("simdjson error {}, key='{}'", simdjson::error_message(err), key));
    }
    return *this;
  }

  template <NoneOfTheAbove T>
  SimdJsonInputArchive& Serialize(const char* key, T& value)
  {
    simdjson::dom::object objAtKey;
    auto err = input.at_key(key).get_object().get(objAtKey);
    if (err) {
      throw std::runtime_error(fmt::format("simdjson error {}, key='{}'", simdjson::error_message(err), key));
    }
    SimdJsonInputArchive childArchive(objAtKey);
    value.Serialize(childArchive);
    return *this;
  }

private:
  // simdjson::dom::object?
  const simdjson::dom::element& input;
  // const simdjson::dom::object& input;
};
