#pragma once
#include "IDatabase.h"

class MigrationDatabase : public IDatabase
{
public:
  MigrationDatabase(std::shared_ptr<IDatabase> newDatabase,
                    std::shared_ptr<IDatabase> oldDatabase);
  size_t Upsert(const std::vector<MpChangeForm>& changeForms) override;
  std::optional<MpChangeForm> FindOne(const FormDesc& formDesc) override;
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
