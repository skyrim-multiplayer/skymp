#include "SaveStorageFactory.h"

#include "AsyncSaveStorage.h"

std::shared_ptr<ISaveStorage> SaveStorageFactory::Create(
  std::shared_ptr<IDatabase> db, std::shared_ptr<spdlog::logger> logger)
{
  return std::make_shared<AsyncSaveStorage>(db, logger);
}
