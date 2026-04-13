#pragma once
#include "MpChangeForms.h"
#include <database_drivers/IDatabase.h>
#include <spdlog/spdlog.h>

class FileDatabase : public Viet::IDatabase<MpChangeForm, FormDesc>
{
public:
  FileDatabase(std::string directory_,
               std::shared_ptr<spdlog::logger> logger_);

  void Iterate(const IterateCallback& iterateCallback,
               std::optional<std::vector<FormDesc>> filter) override;

private:
  std::vector<std::optional<MpChangeForm>>&& UpsertImpl(
    std::vector<std::optional<MpChangeForm>>&& changeForms,
    size_t& outNumUpserted) override;

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
