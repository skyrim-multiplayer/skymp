#pragma once
#include "IDatabase.h"
#include <memory>

namespace mongocxx::v_noabi {
class client_session; // fwd
}

class MongoDatabase : public IDatabase
{
public:
  MongoDatabase(std::string uri_, std::string name_);
  UpsertResult Upsert(
    std::vector<std::optional<MpChangeForm>>&& changeForms) override;
  void Iterate(const IterateCallback& iterateCallback) override;

  UpsertResult DbHash();

private:
  UpsertResult DbHashImpl(
    std::optional<mongocxx::v_noabi::client_session*> existingSession);

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
