#include "FileDatabase.h"
#include "MigrationDatabase.h"
#include "TestUtils.hpp"
#include <catch2/catch.hpp>

inline std::shared_ptr<IDatabase> MakeDatabase(const char* directory);

inline MpChangeForm CreateChangeForm_(const char* descStr,
                                      NiPoint3 pos = { 0, 0, 0 });

inline std::set<MpChangeForm> GetAllChangeForms(std::shared_ptr<IDatabase> db);

