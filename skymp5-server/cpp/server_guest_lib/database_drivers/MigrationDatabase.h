#pragma once
#include "IDatabase.h"

class MigrationDatabase : public IDatabase
{
public:
  MigrationDatabase(
    std::shared_ptr<IDatabase> newDatabase,
    std::shared_ptr<IDatabase> oldDatabase,
    std::function<void()> exit = [] { std::exit(0); },
    std::function<void()> terminate = [] { std::terminate(); });
  size_t Upsert(const std::vector<MpChangeForm>& changeForms) override;
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
