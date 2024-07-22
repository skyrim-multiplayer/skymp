#include "DatabaseFactory.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "FileDatabase.h"
#include "MigrationDatabase.h"
#include "MongoDatabase.h"
#include "ZipDatabase.h"

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

    std::optional<std::string> databaseRedisCacheUri =
      settings.count("databaseRedisCacheUri")
      ? std::optional<std::string>(
          settings["databaseRedisCacheUri"].get<std::string>())
      : std::nullopt;

    auto databaseUri = settings["databaseUri"].get<std::string>();
    logger->info("Using mongodb with name '" + databaseName + "'");
    return std::make_shared<MongoDatabase>(databaseUri, databaseName,
                                           databaseRedisCacheUri);
  }

  if (databaseDriver == "migration") {
    auto from = settings.at("databaseOld");
    auto to = settings.at("databaseNew");
    auto oldDatabase = Create(from, logger);
    auto newDatabase = Create(to, logger);
    return std::make_shared<MigrationDatabase>(newDatabase, oldDatabase);
  }

  if (databaseDriver == "zip") {
    auto databaseName = settings.count("databaseName")
      ? settings["databaseName"].get<std::string>()
      : std::string("world");

    logger->info("Using zip with name '" + databaseName + "'");
    return std::make_shared<ZipDatabase>(databaseName, logger);
  }

  throw std::runtime_error("Unrecognized databaseDriver: " + databaseDriver);
}
