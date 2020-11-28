#pragma once
#include "AsyncSaveStorage.h"
#include "ISaveStorage.h"
#include <functional>
#include <memory>
#include <spdlog/logger.h>
#include <string>
#include <vector>

class SqliteSaveStorage : public AsyncSaveStorage
{
public:
  // logger must support multithreaded writing
  SqliteSaveStorage(std::string filename,
                    std::shared_ptr<spdlog::logger> logger = nullptr);

private:
  std::shared_ptr<IDatabase> CreateDbImpl(std::string filename);
};