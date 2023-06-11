#pragma once
#include "IDatabase.h"
#include <spdlog/spdlog.h>

class FileDatabase : public IDatabase
{
public:
  FileDatabase(std::string directory_,
               std::shared_ptr<spdlog::logger> logger_);

  size_t Upsert(const std::vector<MpChangeForm>& changeForms) override;
  std::optional<MpChangeForm> FindOne(const FormDesc &formDesc) override;
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
