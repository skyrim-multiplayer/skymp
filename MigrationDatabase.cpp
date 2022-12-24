#include "MigrationDatabase.h"
#include <set>

struct MigrationDatabase::Impl
{
  std::shared_ptr<IDatabase> newDatabase;
  std::shared_ptr<IDatabase> oldDatabase;
};

MigrationDatabase::MigrationDatabase(std::shared_ptr<IDatabase> newDatabase,
                                     std::shared_ptr<IDatabase> oldDatabase)
{
  pImpl.reset(new Impl{ newDatabase, oldDatabase });
}

size_t MigrationDatabase::Upsert(const std::vector<MpChangeForm>& changeForms)
{
  return pImpl->newDatabase->Upsert(changeForms);
}

void MigrationDatabase::Iterate(const IterateCallback& iterateCallback)
{
  std::set<FormDesc> alreadyMigrated;

  pImpl->newDatabase->Iterate([&](const MpChangeForm& changeForm) {
    iterateCallback(changeForm);
    alreadyMigrated.insert(changeForm.formDesc);
  });

  pImpl->oldDatabase->Iterate([&](const MpChangeForm& changeForm) {
    if (!alreadyMigrated.count(changeForm.formDesc)) {
      iterateCallback(changeForm);
    }
  });
}
