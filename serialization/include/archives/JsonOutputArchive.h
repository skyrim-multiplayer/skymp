#pragma once
#include "concepts/Concepts.h"
#include <nlohmann/json.hpp>
#include <vector>

struct JsonOutputArchive
{
  nlohmann::json j = nlohmann::json::object();

  template <IntegralConstant T>
  JsonOutputArchive& Serialize(const char* key, const T& value)
  {
    j[key] = T::value;
    return *this;
  }

  template <StringLike T>
  JsonOutputArchive& Serialize(const char* key, const T& value)
  {
    j[key] = value;
    return *this;
  }

  template <ContainerLike T>
  JsonOutputArchive& Serialize(const char* key, const T& value)
  {
    nlohmann::json arr = nlohmann::json::array();
    for (auto& element : value) {
      JsonOutputArchive childArchive;
      childArchive.Serialize("element", element);
      arr.push_back(childArchive.j["element"]);
    }
    j[key] = arr;
    return *this;
  }

  template <Optional T>
  JsonOutputArchive& Serialize(const char* key, const T& value)
  {
    if (value.has_value()) {
      Serialize(key, *value);
    } else {
      j[key] = nlohmann::json{};
    }
    return *this;
  }

  template <Arithmetic T>
  JsonOutputArchive& Serialize(const char* key, const T& value)
  {
    j[key] = value;
    return *this;
  }

  template <SerializableStruct T>
  JsonOutputArchive& Serialize(const char* key, const T& value)
  {
    nlohmann::json arr = nlohmann::json::object();

    JsonOutputArchive childArchive;
    value.Serialize(childArchive);

    j[key] = childArchive.j;
    return *this;
  }
};
