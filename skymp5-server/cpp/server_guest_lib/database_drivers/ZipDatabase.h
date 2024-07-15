#pragma once
#include "IDatabase.h"
#include <spdlog/spdlog.h>

class ZipDatabase : public IDatabase
{
public:
  ZipDatabase(std::string filePath_, std::shared_ptr<spdlog::logger> logger_);
  ~ZipDatabase();

  size_t Upsert(
    std::vector<std::optional<MpChangeForm>>&& changeForms) override;
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
