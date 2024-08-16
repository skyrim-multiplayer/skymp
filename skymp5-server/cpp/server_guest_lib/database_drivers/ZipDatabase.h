#pragma once
#include "IDatabase.h"
#include <spdlog/spdlog.h>

class ZipDatabase : public IDatabase
{
public:
  ZipDatabase(std::string filePath_, std::shared_ptr<spdlog::logger> logger_);
  ~ZipDatabase();

  void Iterate(const IterateCallback& iterateCallback) override;

private:
  std::vector<std::optional<MpChangeForm>>&& UpsertImpl(
    std::vector<std::optional<MpChangeForm>>&& changeForms, size_t &outNumUpserted) override;

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
