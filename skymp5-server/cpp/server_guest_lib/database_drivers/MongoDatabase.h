#pragma once
#include "IDatabase.h"
#include <memory>

class MongoDatabase : public IDatabase
{
public:
  MongoDatabase(std::string uri_, std::string name_,
                std::optional<std::string> redisUri_);
  size_t Upsert(
    std::vector<std::optional<MpChangeForm>>&& changeForms) override;
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  enum class MongoUpsertTransactionMode
  {
    kAppendVersion,
    kReplaceVersion,
  };

  size_t MongoUpsertTransaction(
    std::vector<std::optional<MpChangeForm>>&& changeForms,
    const std::string& changeFormsVersion, MongoUpsertTransactionMode mode);

  void RedisMsetChangeForms(
    const std::vector<std::optional<MpChangeForm>>& changeForms,
    const std::string& changeFormsVersion);

  std::string GetCurrentTimestampIso8601();

  std::string MakeChangeFormRedisKey(const MpChangeForm& changeForm);
  std::string MakeChangeFormRedisKeyWildcard();

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
