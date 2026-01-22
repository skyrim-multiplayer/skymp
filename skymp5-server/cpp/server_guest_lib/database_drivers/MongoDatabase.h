#pragma once
#include "MpChangeForms.h"
#include "utils/JsonSanitizer.h"
#include <database_drivers/IDatabase.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <simdjson.h>

class MongoDatabase : public Viet::IDatabase<MpChangeForm>
{
public:
  MongoDatabase(const std::string& uri_, const std::string& name_);

  // IDatabase
  void Iterate(const IterateCallback& iterateCallback,
               std::optional<std::vector<MpChangeForm>> filter) override;

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
