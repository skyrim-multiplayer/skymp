#include "SqliteDatabase.h"
#include "SqliteChangeForm.h"

namespace {
inline void Throw()
{
  throw std::runtime_error("SQLite support has been dropped. Consider "
                           "selecting a different database.");
}
}

SqliteDatabase::SqliteDatabase(std::string filename_)
  : filename(filename_)
{
  Throw();
}

size_t SqliteDatabase::Upsert(const std::vector<MpChangeForm>& changeForms)
{
  Throw();
  return 0;
}

void SqliteDatabase::Iterate(const IterateCallback& iterateCallback)
{
  Throw();
}
