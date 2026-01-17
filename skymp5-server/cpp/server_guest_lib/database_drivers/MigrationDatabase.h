#pragma once
#include "MpChangeForms.h"
#include <database_drivers/IDatabase.h>

class MigrationDatabase : public IDatabase<MpChangeForm>
{
public:
  MigrationDatabase(
    std::shared_ptr<IDatabase<MpChangeForm>> newDatabase,
    std::shared_ptr<IDatabase<MpChangeForm>> oldDatabase,
    std::function<void()> exit = [] { std::exit(0); },
    std::function<void()> terminate = [] { std::terminate(); });
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  std::vector<std::optional<MpChangeForm>>&& UpsertImpl(
    std::vector<std::optional<MpChangeForm>>&& changeForms,
    size_t& outNumUpserted) override;

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
