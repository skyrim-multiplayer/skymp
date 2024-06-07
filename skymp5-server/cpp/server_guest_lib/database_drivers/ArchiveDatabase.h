#pragma once
#include "IDatabase.h"
#include <spdlog/spdlog.h>

// This is for internal use only and not referenced in the server settings at
// this time.
class ArchiveDatabase : public IDatabase
{
public:
  ArchiveDatabase(std::string filePath_,
                  std::shared_ptr<spdlog::logger> logger_);
  ~ArchiveDatabase();

  UpsertResult Upsert(
    std::vector<std::optional<MpChangeForm>>&& changeForms) override;
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
