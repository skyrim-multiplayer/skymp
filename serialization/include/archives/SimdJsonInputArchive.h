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
    : j_(j)
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
  SimdJsonInputArchive& Serialize(const char* key, T& value)
  {
    // auto result = j_.at_key(key).get_string();
    // result.value();
    simdjson::error_code err = j_.at_key(key).get(value);
    if (err) {
      throw std::runtime_error(fmt::format("simdjson error {}, key='{}'", simdjson::error_message(err), key));
    }
    return *this;
  }

  template <typename T, std::size_t N>
  SimdJsonInputArchive& Serialize(const char* key, std::array<T, N>& value)
  {
    // const auto& arr = j_.at(key);
    // if (arr.size() != N) {
    //   throw std::runtime_error(
    //     "JSON array size does not match std::array size.");
    // }

    // nlohmann::json childArchiveInput = nlohmann::json::object();
    // for (size_t i = 0; i < N; ++i) {
    //   childArchiveInput["element"] = arr.at(i);
    //   SimdJsonInputArchive childArchive(childArchiveInput);
    //   childArchive.Serialize("element", value[i]);
    // }
    return *this;
  }

  template <ContainerLike T>
  SimdJsonInputArchive& Serialize(const char* key, T& value)
  {
    value.clear();

    // const auto& arr = j_.at(key);
    // nlohmann::json childArchiveInput = nlohmann::json::object();
    // for (auto& elementJson : arr) {
    //   typename T::value_type element;
    //   childArchiveInput["element"] = elementJson;
    //   SimdJsonInputArchive childArchive(childArchiveInput);
    //   childArchive.Serialize("element", element);
    //   value.insert(value.end(), element);
    // }
    return *this;
  }

  template <Optional T>
  SimdJsonInputArchive& Serialize(const char* key, T& value)
  {
    // auto it = j_.find(key);
    // if (it != j_.end() && !it->is_null()) {
    //   typename T::value_type actualValue;
    //   Serialize(key, actualValue);
    //   value = actualValue;
    // } else {
    //   value = std::nullopt;
    // }
    return *this;
  }

  template <Arithmetic T>
  SimdJsonInputArchive& Serialize(const char* key, T& value)
  {
    // value = j_.at(key).get<T>();
    // j_.at_pointer()
    j_.at_key(key).get(value);
    return *this;
  }

  template <NoneOfTheAbove T>
  SimdJsonInputArchive& Serialize(const char* key, T& value)
  {
    simdjson::dom::object objAtKey;
    simdjson::error_code err = j_.at_key(key).get_object().get(objAtKey);
    if (err) {
      throw std::runtime_error(fmt::format("simdjson error {}, key='{}'", simdjson::error_message(err), key));
    }
    SimdJsonInputArchive childArchive(objAtKey);
    value.Serialize(childArchive);
    return *this;
  }

private:
  // simdjson::dom::object?
  const simdjson::dom::element& j_;
};
