#pragma once
#include "AsyncSaveStorage.h"
#include "ISaveStorage.h"

class MongoSaveStorage : public AsyncSaveStorage
{
public:
  // logger must support multithreaded writing
  MongoSaveStorage(std::string uri, std::string name,
                   std::shared_ptr<spdlog::logger> logger = nullptr);

private:
  std::shared_ptr<IDatabase> CreateDbImpl(std::string uri, std::string name);
};