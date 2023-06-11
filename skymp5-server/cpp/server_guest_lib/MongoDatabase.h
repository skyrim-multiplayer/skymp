#pragma once
#include "IDatabase.h"
#include <memory>

class MongoDatabase : public IDatabase
{
public:
  MongoDatabase(std::string uri_, std::string name_);
  size_t Upsert(const std::vector<MpChangeForm>& changeForms) override;
  std::optional<MpChangeForm> FindOne(const FormDesc &formDesc) override;
  void Iterate(const IterateCallback& iterateCallback) override;

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
