#pragma once
#include "MpChangeForms.h"
#include <database_drivers/IDatabase.h>
#include <spdlog/spdlog.h>

class ZipDatabase : public Viet::IDatabase<MpChangeForm>
{
public:
  ZipDatabase(std::string filePath_, std::shared_ptr<spdlog::logger> logger_);
  ~ZipDatabase();

  void Iterate(const IterateCallback& iterateCallback,
               std::optional<std::vector<MpChangeForm>> filter) override;

private:
  std::vector<std::optional<MpChangeForm>>&& UpsertImpl(
    std::vector<std::optional<MpChangeForm>>&& changeForms,
    size_t& outNumUpserted) override;

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
