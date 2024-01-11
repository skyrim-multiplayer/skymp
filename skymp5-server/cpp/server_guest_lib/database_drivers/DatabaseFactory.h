#pragma once
#include "IDatabase.h"
#include <memory>
#include <spdlog/spdlog.h>

class DatabaseFactory
{
public:
  static std::shared_ptr<IDatabase> Create(
    nlohmann::json settings, std::shared_ptr<spdlog::logger> logger);
};
