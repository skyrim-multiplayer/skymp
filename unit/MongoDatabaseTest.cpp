#include "database_drivers/MongoDatabase.h"
#include "TestUtils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <simdjson.h>

namespace MongoDatabaseTestUtils {
std::string BytesToHexString(const uint8_t* bytes, size_t length)
{
  static constexpr auto kHexDigits = "0123456789abcdef";

  std::string hexStr(length * 2, ' ');
  for (size_t i = 0; i < length; ++i) {
    hexStr[2 * i] = kHexDigits[(bytes[i] >> 4) & 0xF];
    hexStr[2 * i + 1] = kHexDigits[bytes[i] & 0xF];
  }

  return hexStr;
}

std::string Sha256(const std::string& str)
{
  uint8_t hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const uint8_t*>(str.data()), str.size(), hash);
  return BytesToHexString(hash, SHA256_DIGEST_LENGTH);
}
}

TEST_CASE("JsonSanitizer can restore documents sanitized by an old sanitizer",
          "[MongoDatabase]")
{
  JsonSanitizer sanitizerOld({ '\0', '$', '.' },
                             MongoDatabaseTestUtils::Sha256);
  JsonSanitizer sanitizerNew({ '\0' }, MongoDatabaseTestUtils::Sha256);

  simdjson::dom::parser parser;

  nlohmann::json original = { { "key.with.dot", "value1" },
                              { "$key_with_dollar", 42 },
                              { "valid_key", "value3" } };

  nlohmann::json sanitizedByOld = sanitizerOld.SanitizeJsonRecursive(original);

  std::string sanitizedStr = sanitizedByOld.dump();
  simdjson::dom::element element = parser.parse(sanitizedStr).value();

  bool wasRestored = false;
  nlohmann::json restoredByNew =
    sanitizerNew.RestoreSanitizedJsonRecursive(element, wasRestored);

  REQUIRE(restoredByNew == original);
  REQUIRE(wasRestored == true);
}

TEST_CASE("JsonSanitizer Key and JSON Sanitization", "[MongoDatabase]")
{
  JsonSanitizer sanitizer({ '\0', '$', '.' }, MongoDatabaseTestUtils::Sha256);

  simdjson::dom::parser parser;

  SECTION("SanitizeKey correctly identifies keys that need sanitization")
  {
    // Valid keys should not be sanitized
    REQUIRE(sanitizer.SanitizeKey("validKey") == std::nullopt);
    REQUIRE(sanitizer.SanitizeKey("valid_key_123") == std::nullopt);

    // Keys with banned characters ('.', '$', '\0') must be sanitized
    REQUIRE(sanitizer.SanitizeKey("key.with.dot").has_value());
    REQUIRE(sanitizer.SanitizeKey("$key_with_dollar").has_value());

    std::string keyWithNull = "key!with_null";
    keyWithNull[3] = '\0';
    REQUIRE(sanitizer.SanitizeKey(keyWithNull).has_value());

    // Empty keys must be sanitized
    REQUIRE(sanitizer.SanitizeKey("").has_value());

    // Keys that start with the reserved prefix must be sanitized to avoid
    // collision
    std::string prefixKey = sanitizer.GetEncPrefix() + "some_hash_value";
    REQUIRE(sanitizer.SanitizeKey(prefixKey).has_value());

    // Ensure sanitization is deterministic and produces different hashes for
    // different keys
    auto hash1 = sanitizer.SanitizeKey("key.with.dot");
    auto hash2 = sanitizer.SanitizeKey("$key_with_dollar");
    auto hash3 = sanitizer.SanitizeKey("key.with.dot");
    REQUIRE(hash1 != hash2);
    REQUIRE(hash1 == hash3);
  }

  SECTION("SanitizeJsonRecursive and RestoreSanitizedJsonRecursive round-trip")
  {
    // Helper lambda to perform a full sanitize -> restore -> verify cycle
    auto perform_round_trip_check = [&](const nlohmann::json& original,
                                        bool expect_restoration) {
      // 1. Sanitize the original JSON
      nlohmann::json sanitized = sanitizer.SanitizeJsonRecursive(original);

      // 2. Convert to string and parse with simdjson, as required by the
      // Restore function
      std::string sanitized_str = sanitized.dump();
      simdjson::dom::element element = parser.parse(sanitized_str).value();

      // 3. Restore the sanitized JSON
      bool was_restored = false;
      nlohmann::json restored =
        sanitizer.RestoreSanitizedJsonRecursive(element, was_restored);

      // 4. Verify the restored JSON matches the original and the restoration
      // flag is correct
      REQUIRE(restored == original);
      REQUIRE(was_restored == expect_restoration);
    };

    SECTION("JSON with only valid keys should not be modified")
    {
      nlohmann::json original = { { "key1", "value1" },
                                  { "key2", { { "nested_key", 123 } } },
                                  { "key3", { 1, 2, "three" } } };
      perform_round_trip_check(original, false);
    }

    SECTION("JSON with invalid top-level keys should be restored correctly")
    {
      nlohmann::json original = { { "key.with.dot", "value1" },
                                  { "$key_with_dollar", 42 },
                                  { "", "empty_key_value" } };
      perform_round_trip_check(original, true);
    }

    SECTION("JSON with invalid nested keys should be restored correctly")
    {
      nlohmann::json original = {
        { "valid_key",
          { { "nested.invalid.key", "some_value" },
            { "another_valid_key",
              { { "$another_nested_invalid", true } } } } }
      };
      perform_round_trip_check(original, true);
    }

    SECTION("JSON with objects containing invalid keys inside arrays")
    {
      nlohmann::json original = {
        { "array_key",
          { { { "valid_key", "val" } },    // No sanitization needed here
            { { "invalid.key", "val2" } }, // Sanitization needed
            { { { "$$$", 123 } } } } }     // Sanitization needed
      };
      perform_round_trip_check(original, true);
    }

    SECTION("JSON with a key that starts with the reserved prefix")
    {
      std::string reserved_key = sanitizer.GetEncPrefix() + "some_value";
      nlohmann::json original = {
        { reserved_key, "this key should also be sanitized and restored" }
      };
      perform_round_trip_check(original, true);
    }

    SECTION("A complex JSON object with mixed valid and invalid keys")
    {
      nlohmann::json original = {
        { "regularKey", "value" },
        { "key.with.dots", 123 },
        { "$keyWithDollar", true },
        { "", nullptr },
        { "nestedObject",
          { { "anotherRegularKey", "nested value" },
            { "another.invalid.key", { 1, 2, 3 } } } },
        { "arrayWithObjects", { { { "valid", 1 } }, { { "invalid$", 2 } } } }
      };
      perform_round_trip_check(original, true);
    }

    SECTION("Verify the structure of a sanitized object")
    {
      nlohmann::json original = { { "a.b", 1 }, { "c", 2 } };
      nlohmann::json sanitized = sanitizer.SanitizeJsonRecursive(original);

      // It should contain the original valid key 'c'
      REQUIRE(sanitized.contains("c"));
      REQUIRE(sanitized["c"] == 2);

      // It should contain the special map for encoded keys
      REQUIRE(sanitized.contains(sanitizer.GetEncKeysKey()));
      nlohmann::json enc_keys = sanitized[sanitizer.GetEncKeysKey()];
      REQUIRE(enc_keys.is_object());
      REQUIRE(enc_keys.size() == 1);

      // Find the hashed key and verify its mapping and value
      std::string hashed_key;
      for (auto& [key, value] : sanitized.items()) {
        if (key.starts_with(sanitizer.GetEncPrefix())) {
          hashed_key = key;
          break;
        }
      }
      REQUIRE(!hashed_key.empty());
      REQUIRE(sanitized.contains(hashed_key));
      REQUIRE(sanitized[hashed_key] == 1);
      REQUIRE(enc_keys[hashed_key] == "a.b");
    }
  }
}
