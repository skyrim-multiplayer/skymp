#pragma once
#include "IDatabase.h"

class SqliteDatabase : public IDatabase
{
public:
  SqliteDatabase(std::string filename_);
  size_t Upsert(const std::vector<MpChangeForm>& changeForms) override;
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  const std::string filename;
};
