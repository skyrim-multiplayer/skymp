#pragma once
#include "IDatabase.h"
#include <memory>

class IKeySanitizer
{
public:
  virtual const std::string& GetEncKeysKey() const noexcept = 0;
  virtual const std::string& GetEncPrefix() const noexcept = 0;
  virtual std::optional<std::string> SanitizeKey(const std::string& key) = 0;
  virtual nlohmann::json SanitizeJsonRecursive(const nlohmann::json& j) = 0;
  virtual nlohmann::json RestoreSanitizedJsonRecursive(
    simdjson::dom::element element, bool& restored) = 0;
};

class MongoDatabase
  : public IDatabase
  , public IKeySanitizer
{
public:
  MongoDatabase(std::string uri_, std::string name_);

  // IDatabase
  void Iterate(const IterateCallback& iterateCallback) override;

  // IKeySanitizer
  const std::string& GetEncKeysKey() const noexcept override;
  const std::string& GetEncPrefix() const noexcept override;
  std::optional<std::string> SanitizeKey(const std::string& key) override;
  nlohmann::json SanitizeJsonRecursive(const nlohmann::json& j) override;
  nlohmann::json RestoreSanitizedJsonRecursive(simdjson::dom::element element,
                                               bool& restored) override;

private:
  std::vector<std::optional<MpChangeForm>>&& UpsertImpl(
    std::vector<std::optional<MpChangeForm>>&& changeForms,
    size_t& outNumUpserted) override;

  int GetDocumentCount();
  std::optional<std::string> GetCombinedErrorOrNull(
    const std::vector<std::optional<std::string>>& errorList);

  std::string BytesToHexString(const uint8_t* bytes, size_t length);
  std::string Sha256(const std::string& str);

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
