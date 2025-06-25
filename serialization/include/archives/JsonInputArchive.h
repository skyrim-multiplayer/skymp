#pragma once
#include "concepts/Concepts.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

class JsonInputArchive
{
public:
  explicit JsonInputArchive(const nlohmann::json& json)
    : j(json)
  {
  }

  template <IntegralConstant T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    // Compile time constant. Do nothing
    // Maybe worth adding equality check
    return *this;
  }

  template <StringLike T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    value = j.at(key).get<std::string>();
    return *this;
  }

  // Specialization for std::array
  template <typename T, std::size_t N>
  JsonInputArchive& Serialize(const char* key, std::array<T, N>& value)
  {
    const auto& arr = j.at(key);
    if (arr.size() != N) {
      throw std::runtime_error(
        "JSON array size does not match std::array size.");
    }

    nlohmann::json childArchiveInput = nlohmann::json::object();
    for (size_t i = 0; i < N; ++i) {
      childArchiveInput["element"] = arr.at(i);
      JsonInputArchive childArchive(childArchiveInput);
      childArchive.Serialize("element", value[i]);
    }
    return *this;
  }

  template <ContainerLike T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    value.clear();

    const auto& arr = j.at(key);
    nlohmann::json childArchiveInput = nlohmann::json::object();
    for (auto& elementJson : arr) {
      typename T::value_type element;
      childArchiveInput["element"] = elementJson;
      JsonInputArchive childArchive(childArchiveInput);
      childArchive.Serialize("element", element);
      value.insert(value.end(), element);
    }
    return *this;
  }

  template <Optional T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) {
      typename T::value_type actualValue;
      Serialize(key, actualValue);
      value = actualValue;
    } else {
      value = std::nullopt;
    }
    return *this;
  }

  template <Arithmetic T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    value = j.at(key).get<T>();
    return *this;
  }

  template <typename... Types>
  JsonInputArchive& Serialize(const char* key, std::variant<Types...>& value)
  {
    const auto& jsonValue = j.at(key);

    // Helper lambda to attempt deserialization for a specific type
    auto tryDeserialize = [&](auto typeTag) -> bool {
      using SelectedType = decltype(typeTag);

      nlohmann::json childArchiveInput = nlohmann::json::object();
      childArchiveInput["candidate"] = jsonValue;
      SelectedType deserializedValue;
      JsonInputArchive childArchive(childArchiveInput);

      bool deserializationSuccessful = false;
      try {
        childArchive.Serialize("candidate", deserializedValue);
        deserializationSuccessful = true;
      } catch (const std::exception&) {
        deserializationSuccessful = false;
      }

      if (deserializationSuccessful) {
        value = std::move(deserializedValue);
      }

      return deserializationSuccessful;
    };

    // Iterate through the variant types and attempt deserialization
    bool success = false;
    [&]<std::size_t... Is>(std::index_sequence<Is...>)
    {
      ((success = success ||
          tryDeserialize(std::declval<typename std::variant_alternative<
                           Is, std::variant<Types...>>::type>())),
       ...);
    }
    (std::make_index_sequence<sizeof...(Types)>{});

    if (!success) {
      throw std::runtime_error(
        "Unable to deserialize JSON into any variant type");
    }

    return *this;
  }

  template <NoneOfTheAbove T>
  JsonInputArchive& Serialize(const char* key, T& value)
  {
    const auto& obj = j.at(key);
    JsonInputArchive childArchive(obj);
    value.Serialize(childArchive);
    return *this;
  }

private:
  const nlohmann::json& j;
};
