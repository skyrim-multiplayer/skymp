#include "MigrationDatabase.h"
#include <set>
#include <spdlog/spdlog.h>

namespace {
size_t CountChangeForms(std::shared_ptr<IDatabase> database)
{
  size_t n = 0;
  database->Iterate([&](const MpChangeForm&) { ++n; });
  return n;
}
}

struct MigrationDatabase::Impl
{
  std::shared_ptr<IDatabase> newDatabase;
  std::shared_ptr<IDatabase> oldDatabase;
};

MigrationDatabase::MigrationDatabase(std::shared_ptr<IDatabase> newDatabase,
                                     std::shared_ptr<IDatabase> oldDatabase)
{
  pImpl.reset(new Impl{ newDatabase, oldDatabase });

  spdlog::info("MigrationDatabase: verifying newDatabase emptiness");

  size_t newDatabaseCount = CountChangeForms(newDatabase);

  if (newDatabaseCount > 0) {
    spdlog::error(
      "MigrationDatabase: newDatabase is not empty, skipping migration");
    spdlog::info("The server will be terminated");
    std::terminate();
    return;
  }

  spdlog::info("MigrationDatabase: verifying oldDatabase non-emptiness");

  if (CountChangeForms(oldDatabase) == 0) {
    spdlog::error(
      "MigrationDatabase: oldDatabase is empty, skipping migration");
    spdlog::info("The server will be terminated");
    std::terminate();
    return;
  }

  spdlog::info("MigrationDatabase: preparing changeforms to migrate");

  std::vector<MpChangeForm> changeForms;

  uint32_t counter = 0;

  oldDatabase->Iterate([&](const MpChangeForm& changeForm) {
    changeForms.push_back(changeForm);
    ++counter;
    if (counter <= 100) {
      if (counter % 10 == 0) {
        spdlog::info("MigrationDatabase: prepared {} changeforms", counter);
      }
    } else if (counter <= 1000) {
      if (counter % 100 == 0) {
        spdlog::info("MigrationDatabase: prepared {} changeforms", counter);
      }
    } else {
      if (counter % 1000 == 0) {
        spdlog::info("MigrationDatabase: prepared {} changeforms", counter);
      }
    }
  });

  spdlog::info("MigrationDatabase: upserting {} changeforms into the new "
               "database, this may take time",
               changeForms.size());

  size_t numUpserted = newDatabase->Upsert(changeForms);

  spdlog::info("MigrationDatabase: {} changeForms migrated successfully",
               numUpserted);
  spdlog::info("The server will be terminated");
  std::terminate();
}

size_t MigrationDatabase::Upsert(const std::vector<MpChangeForm>& changeForms)
{
  spdlog::error("MigrationDatabase::Upsert - should never be reached");
  std::terminate();
}

void MigrationDatabase::Iterate(const IterateCallback& iterateCallback)
{
  spdlog::error("MigrationDatabase::Iterate - should never be reached");
  std::terminate();
}
