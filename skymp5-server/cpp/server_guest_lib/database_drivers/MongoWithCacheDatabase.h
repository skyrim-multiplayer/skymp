#pragma once
#include "IDatabase.h"
#include <memory>
#include <spdlog/spdlog.h>

class MongoWithCacheDatabase : public IDatabase
{
public:
  MongoWithCacheDatabase(std::string uri_, std::string name_,
                         std::shared_ptr<spdlog::logger> logger_);
  UpsertResult Upsert(
    std::vector<std::optional<MpChangeForm>>&& changeForms) override;
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  void EnsureArchiveMatchesCrc32();
  void EnsureArchiveMatchesMongoDbHash();

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
