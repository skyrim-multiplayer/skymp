#include "DatabaseFactory.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "FileDatabase.h"
#include "MigrationDatabase.h"
#include "MongoDatabase.h"

std::shared_ptr<IDatabase> DatabaseFactory::Create(
  nlohmann::json settings, std::shared_ptr<spdlog::logger> logger)
{
  auto databaseDriver = settings.count("databaseDriver")
    ? settings["databaseDriver"].get<std::string>()
    : std::string("file");

  if (databaseDriver == "file") {
    auto databaseName = settings.count("databaseName")
      ? settings["databaseName"].get<std::string>()
      : std::string("world");

    logger->info("Using file with name '" + databaseName + "'");
    return std::make_shared<FileDatabase>(databaseName, logger);
  }

  if (databaseDriver == "mongodb") {
    auto databaseName = settings.count("databaseName")
      ? settings["databaseName"].get<std::string>()
      : std::string("db");

    auto databaseUri = settings["databaseUri"].get<std::string>();
    logger->info("Using mongodb with name '" + databaseName + "'");
    return std::make_shared<MongoDatabase>(databaseUri, databaseName);
  }

  if (databaseDriver == "migration") {
    auto from = settings.at("databaseOld");
    auto to = settings.at("databaseNew");
    auto oldDatabase = Create(from, logger);
    auto newDatabase = Create(to, logger);
    return std::make_shared<MigrationDatabase>(newDatabase, oldDatabase);
  }

  throw std::runtime_error("Unrecognized databaseDriver: " + databaseDriver);
}
