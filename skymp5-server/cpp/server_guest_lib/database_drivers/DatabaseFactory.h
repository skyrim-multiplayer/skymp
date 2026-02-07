#pragma once
#include "MpChangeForms.h"
#include <database_drivers/IDatabase.h>
#include <memory>
#include <spdlog/spdlog.h>

class DatabaseFactory
{
public:
  static std::shared_ptr<Viet::IDatabase<MpChangeForm, FormDesc>> Create(
    nlohmann::json settings, std::shared_ptr<spdlog::logger> logger);
};
