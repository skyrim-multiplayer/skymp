#pragma once
#include "ISaveStorage.h"
#include "database_drivers/IDatabase.h"
#include <memory>
#include <spdlog/spdlog.h>

class SaveStorageFactory
{
public:
  static std::shared_ptr<ISaveStorage> Create(
    std::shared_ptr<IDatabase> db, std::shared_ptr<spdlog::logger> logger);
};
