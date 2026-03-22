#include "JsonSanitizer.h"

JsonSanitizer::JsonSanitizer(
  const std::vector<char>& bannedCharacters_,
  const std::function<std::string(const std::string&)>& hashFunction_)
  : bannedCharacters(bannedCharacters_)
  , hashFunction(hashFunction_)
{
}

const std::string& JsonSanitizer::GetEncKeysKey() const noexcept
{
  static const std::string kEncKeysKey = "_enc_keys";
  return kEncKeysKey;
}

const std::string& JsonSanitizer::GetEncPrefix() const noexcept
{
  static const std::string kEncHashPrefix = "_enc_hash_";
  return kEncHashPrefix;
}

std::optional<std::string> JsonSanitizer::SanitizeKey(const std::string& key)
{
  for (char character : bannedCharacters) {
    auto it = std::find(key.begin(), key.end(), character);
    if (it != key.end()) {
      return GetEncPrefix() + hashFunction(key);
    }
  }

  // Empty keys are banned by MongoDB
  if (key.empty()) {
    return GetEncPrefix() + hashFunction(key);
  }

  // Avoid collisions in case one tries to use GetEncPrefix() +
  // hashFunction(key) as another key
  if (key.starts_with(GetEncPrefix())) {
    return GetEncPrefix() + hashFunction(key);
  }

  return std::nullopt;
}

nlohmann::json JsonSanitizer::SanitizeJsonRecursive(const nlohmann::json& j)
{
  if (j.is_object()) {
    nlohmann::json sanitizedObj = nlohmann::json::object();
    nlohmann::json encodedKeysMap = nlohmann::json::object();

    for (auto& [key, value] : j.items()) {
      std::optional<std::string> newKey = SanitizeKey(key);
      if (newKey.has_value()) {
        encodedKeysMap[*newKey] = key;
        sanitizedObj[*newKey] = SanitizeJsonRecursive(value);
      } else {
        sanitizedObj[key] = SanitizeJsonRecursive(value);
      }
    }

    if (!encodedKeysMap.empty()) {
      sanitizedObj[GetEncKeysKey()] = encodedKeysMap;
    }
    return sanitizedObj;
  }

  if (j.is_array()) {
    nlohmann::json sanitizedArr = nlohmann::json::array();
    for (const auto& item : j) {
      sanitizedArr.push_back(SanitizeJsonRecursive(item));
    }
    return sanitizedArr;
  }

  return j;
}

nlohmann::json JsonSanitizer::RestoreSanitizedJsonRecursive(
  simdjson::dom::element element, bool& restored)
{
  switch (element.type()) {
    case simdjson::dom::element_type::OBJECT: {
      nlohmann::json restoredObj = nlohmann::json::object();
      simdjson::dom::object obj = element.get_object();

      std::map<std::string, std::string> keyMap;
      auto encodedKeysField = obj[GetEncKeysKey()];
      if (!encodedKeysField.error() &&
          encodedKeysField.type() == simdjson::dom::element_type::OBJECT) {
        restored = true;
        for (auto field : encodedKeysField.get_object()) {
          keyMap[std::string(field.key)] =
            std::string(field.value.get_string().value());
        }
      }

      for (auto field : obj) {
        std::string_view key_sv = field.key;
        if (key_sv == GetEncKeysKey()) {
          continue;
        }

        std::string restoredKey(key_sv);
        auto it = keyMap.find(restoredKey);
        if (it != keyMap.end()) {
          restoredKey = it->second;
        }

        restoredObj[restoredKey] =
          RestoreSanitizedJsonRecursive(field.value, restored);
      }
      return restoredObj;
    }

    case simdjson::dom::element_type::ARRAY: {
      nlohmann::json restoredArr = nlohmann::json::array();
      for (simdjson::dom::element child : element.get_array()) {
        restoredArr.push_back(RestoreSanitizedJsonRecursive(child, restored));
      }
      return restoredArr;
    }

    case simdjson::dom::element_type::STRING:
      return std::string(element.get_string().value());
    case simdjson::dom::element_type::INT64:
      return element.get_int64().value();
    case simdjson::dom::element_type::UINT64:
      return element.get_uint64().value();
    case simdjson::dom::element_type::DOUBLE:
      return element.get_double().value();
    case simdjson::dom::element_type::BOOL:
      return element.get_bool().value();
    case simdjson::dom::element_type::NULL_VALUE:
      return nullptr;
    default:
      return nullptr;
  }
}
