#include "MigrationDatabase.h"
#include "FileDatabase.h"
#include "TestUtils.hpp"
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

TEST_CASE("Moves data from one database from another", "[MigrationDatabase]")
{
  auto oldDatabase = MakeDatabase("unit/data/a");
  oldDatabase->Upsert({ CreateChangeForm_("0") });
  oldDatabase->Upsert({ CreateChangeForm_("1") });
  oldDatabase->Upsert({ CreateChangeForm_("2") });

  auto newDatabase = MakeDatabase("unit/data/b");
  std::vector<MpChangeForm> initialNewDatabase = {
    CreateChangeForm_("3"), CreateChangeForm_("4"),
    CreateChangeForm_("0", { 1, 2, 3 })
  };
  newDatabase->Upsert(initialNewDatabase);

  auto db = std::make_shared<MigrationDatabase>(newDatabase, oldDatabase);

  REQUIRE(GetAllChangeForms(db) ==
          std::set<MpChangeForm>(
            { CreateChangeForm_("0", { 1, 2, 3 }), CreateChangeForm_("1"),
              CreateChangeForm_("2"), CreateChangeForm_("3"),
              CreateChangeForm_("4") }));
}
