#pragma once
#include "AsyncSaveStorage.h"
#include "ISaveStorage.h"
#include "database_drivers/IDatabase.h"
#include <memory>
#include <spdlog/spdlog.h>

namespace Viet {

class SaveStorageFactory
{
public:
  template <typename T, typename FormDescType,
            typename ThreadType = std::thread>
  static std::shared_ptr<ISaveStorage<T, FormDescType>> Create(
    std::shared_ptr<IDatabase<T, FormDescType>> db,
    std::shared_ptr<spdlog::logger> logger,
    typename AsyncSaveStorage<T, FormDescType, ThreadType>::ThreadFactory
      threadFactory = {})
  {
    return std::make_shared<AsyncSaveStorage<T, FormDescType, ThreadType>>(
      db, logger, "", std::nullopt, std::move(threadFactory));
  }
};

}
