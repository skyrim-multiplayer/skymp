#pragma once
#include "IDatabase.h"
#include <spdlog/spdlog.h>

class FileDatabase : public IDatabase
{
public:
  FileDatabase(std::string directory_,
               std::shared_ptr<spdlog::logger> logger_);

  void Iterate(const IterateCallback& iterateCallback) override;

private:
  std::vector<std::optional<MpChangeForm>>&& UpsertImpl(
    std::vector<std::optional<MpChangeForm>>&& changeForms,
    size_t& outNumUpserted) override;

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
