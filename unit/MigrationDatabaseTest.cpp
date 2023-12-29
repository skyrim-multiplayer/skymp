#include "database_drivers/MigrationDatabase.h"
#include "TestUtils.hpp"
#include "database_drivers/FileDatabase.h"
#include <catch2/catch_all.hpp>

inline std::shared_ptr<IDatabase> MakeDatabase(const char* directory)
{
  if (std::filesystem::exists(directory)) {
    std::filesystem::remove_all(directory);
  }

  return std::make_shared<FileDatabase>(directory, spdlog::default_logger());
}

inline MpChangeForm CreateChangeForm_(const char* descStr,
                                      NiPoint3 pos = { 0, 0, 0 })
{
  MpChangeForm res;
  res.formDesc = FormDesc::FromString(descStr);
  res.position = pos;
  return res;
}

inline std::set<MpChangeForm> GetAllChangeForms(std::shared_ptr<IDatabase> db)
{
  std::set<MpChangeForm> res;
  db->Iterate([&](const MpChangeForm& changeForm) { res.insert(changeForm); });
  return res;
}

TEST_CASE("Successful Migration", "[MigrationDatabase]")
{
  std::atomic<bool> exited{ false };
  auto customExit = [&exited]() { exited = true; };

  auto oldDatabase = MakeDatabase("unit/data/old");
  oldDatabase->Upsert({ CreateChangeForm_("1") });
  oldDatabase->Upsert({ CreateChangeForm_("2") });

  auto newDatabase = MakeDatabase("unit/data/new");

  auto db =
    std::make_shared<MigrationDatabase>(newDatabase, oldDatabase, customExit);
  REQUIRE(GetAllChangeForms(newDatabase) == GetAllChangeForms(oldDatabase));
  REQUIRE(exited == true); // Check if the custom terminate was called
}

TEST_CASE("Migration Termination on Non-Empty New Database",
          "[MigrationDatabase]")
{
  std::atomic<bool> exited{ false };
  auto customExit = [&exited]() { exited = true; };

  auto oldDatabase = MakeDatabase("unit/data/old");
  oldDatabase->Upsert({ CreateChangeForm_("1") });

  auto newDatabase = MakeDatabase("unit/data/new");
  newDatabase->Upsert({ CreateChangeForm_("3") });

  auto db =
    std::make_shared<MigrationDatabase>(newDatabase, oldDatabase, customExit);

  // Nothing changed
  REQUIRE(GetAllChangeForms(oldDatabase) ==
          std::set<MpChangeForm>{ CreateChangeForm_("1") });
  REQUIRE(GetAllChangeForms(newDatabase) ==
          std::set<MpChangeForm>{ CreateChangeForm_("3") });
  REQUIRE(exited == true); // Check if the custom terminate was called
}

TEST_CASE("Migration Termination on Empty Old Database", "[MigrationDatabase]")
{
  std::atomic<bool> exited{ false };
  auto customExit = [&exited]() { exited = true; };

  auto oldDatabase = MakeDatabase("unit/data/old");
  oldDatabase->Upsert({});

  auto newDatabase = MakeDatabase("unit/data/new");
  newDatabase->Upsert({});

  auto db =
    std::make_shared<MigrationDatabase>(newDatabase, oldDatabase, customExit);

  // Nothing changed
  REQUIRE(GetAllChangeForms(oldDatabase) == std::set<MpChangeForm>{});
  REQUIRE(GetAllChangeForms(newDatabase) == std::set<MpChangeForm>{});
  REQUIRE(exited == true); // Check if the custom terminate was called
}
