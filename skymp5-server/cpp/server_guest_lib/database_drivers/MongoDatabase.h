#pragma once
#include "IDatabase.h"
#include <memory>

class MongoDatabase : public IDatabase
{
public:
  MongoDatabase(std::string uri_, std::string name_);
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  std::vector<std::optional<MpChangeForm>>&& UpsertImpl(
    std::vector<std::optional<MpChangeForm>>&& changeForms,
    size_t& outNumUpserted) override;

  int GetDocumentCount();
  std::optional<std::string> GetCombinedErrorOrNull(
    const std::vector<std::optional<std::string>>& errorList);

  std::string BytesToHexString(const uint8_t* bytes, size_t length);
  std::string Sha256(const std::string& str);

  const std::string& GetEncKeysKey() const noexcept;
  const std::string& GetEncPrefix() const noexcept;
  std::optional<std::string> SanitizeKey(const std::string& key);
  nlohmann::json SanitizeJsonRecursive(const nlohmann::json& j);
  nlohmann::json RestoreSanitizedJsonRecursive(simdjson::dom::element element,
                                               bool& restored);

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
