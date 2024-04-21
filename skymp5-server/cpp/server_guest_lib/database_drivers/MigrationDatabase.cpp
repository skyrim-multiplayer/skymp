#include "MigrationDatabase.h"
#include <algorithm>
#include <iterator>
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
  std::function<void()> exit;
  std::function<void()> terminate;
};

MigrationDatabase::MigrationDatabase(std::shared_ptr<IDatabase> newDatabase,
                                     std::shared_ptr<IDatabase> oldDatabase,
                                     std::function<void()> exit,
                                     std::function<void()> terminate)
{
  pImpl.reset(new Impl{ newDatabase, oldDatabase, exit });

  spdlog::info("MigrationDatabase: verifying newDatabase emptiness");

  size_t newDatabaseCount = CountChangeForms(newDatabase);

  if (newDatabaseCount > 0) {
    spdlog::error(
      "MigrationDatabase: newDatabase is not empty, skipping migration");
    spdlog::info("The server will exit");
    pImpl->exit();
    return;
  }

  spdlog::info("MigrationDatabase: verifying oldDatabase non-emptiness");

  if (CountChangeForms(oldDatabase) == 0) {
    spdlog::error(
      "MigrationDatabase: oldDatabase is empty, skipping migration");
    spdlog::info("The server will exit");
    pImpl->exit();
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

  // Instead of upserting all changeForms at once, split them into chunks of 1k
  // and upsert each chunk
  size_t chunkSize = 1000;
  size_t totalUpserted = 0;

  for (size_t i = 0; i < changeForms.size(); i += chunkSize) {
    std::vector<std::optional<MpChangeForm>> chunk;

    // Calculate the end of the current chunk
    size_t end = std::min(i + chunkSize, changeForms.size());

    // Copy the elements for the current chunk
    std::copy(changeForms.begin() + i, changeForms.begin() + end,
              std::back_inserter(chunk));

    // Upsert the current chunk
    size_t numUpserted = newDatabase->Upsert(std::move(chunk));
    totalUpserted += numUpserted;

    spdlog::info("MigrationDatabase: upserted chunk {}/{} ({} changeForms)",
                 (i / chunkSize) + 1,
                 (changeForms.size() + chunkSize - 1) / chunkSize,
                 numUpserted);
  }

  spdlog::info("MigrationDatabase: {} total changeForms migrated successfully",
               totalUpserted);

  spdlog::info("The server will exit");
  pImpl->exit();
}

size_t MigrationDatabase::Upsert(
  std::vector<std::optional<MpChangeForm>>&& changeForms)
{
  spdlog::error("MigrationDatabase::Upsert - should never be reached");
  pImpl->terminate();
  return 0;
}

void MigrationDatabase::Iterate(const IterateCallback& iterateCallback)
{
  spdlog::error("MigrationDatabase::Iterate - should never be reached");
  pImpl->terminate();
}
