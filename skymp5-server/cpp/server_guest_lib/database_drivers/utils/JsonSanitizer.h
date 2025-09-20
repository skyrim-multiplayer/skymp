#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <simdjson.h>
#include <string>
#include <vector>

class JsonSanitizer
{
public:
  explicit JsonSanitizer(const std::vector<char>& bannedCharacters_);

  const std::string& GetEncKeysKey() const noexcept;
  const std::string& GetEncPrefix() const noexcept;

  nlohmann::json SanitizeJsonRecursive(const nlohmann::json& j);
  nlohmann::json RestoreSanitizedJsonRecursive(simdjson::dom::element element,
                                               bool& restored);
  std::optional<std::string> SanitizeKey(const std::string& key);

private:
  const std::vector<char> bannedCharacters;
};
