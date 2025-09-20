#pragma once
#include "IDatabase.h"
#include "utils/JsonSanitizer.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <simdjson.h>

class MongoDatabase : public IDatabase
{
public:
  MongoDatabase(const std::string& uri_, const std::string& name_);

  // IDatabase
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

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
