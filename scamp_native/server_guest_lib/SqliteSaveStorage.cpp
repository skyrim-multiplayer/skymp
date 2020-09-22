#include "SqliteSaveStorage.h"
#include "SqliteChangeForm.h"
#include <atomic>
#include <list>
#include <mutex>
#include <sqlite_orm/sqlite_orm.h>
#include <thread>

namespace {
auto MakeSqliteStorage(const char* name)
{
  using namespace sqlite_orm;

  auto storage = make_storage(
    name,
    make_table<SqliteChangeForm>(
      "MpChangeForm",
      make_column("primary", &SqliteChangeForm::primary, primary_key()),
      make_column("record_type", &SqliteChangeForm::recType),
      make_column("base_desc", &SqliteChangeForm::GetBaseFormDesc,
                  &SqliteChangeForm::SetBaseFormDesc),
      make_column("form_desc", &SqliteChangeForm::GetFormDesc,
                  &SqliteChangeForm::SetFormDesc),
      make_column("x", &SqliteChangeForm::GetX, &SqliteChangeForm::SetX),
      make_column("y", &SqliteChangeForm::GetY, &SqliteChangeForm::SetY),
      make_column("z", &SqliteChangeForm::GetZ, &SqliteChangeForm::SetZ),
      make_column("angle_x", &SqliteChangeForm::GetAngleX,
                  &SqliteChangeForm::SetAngleX),
      make_column("angle_y", &SqliteChangeForm::GetAngleY,
                  &SqliteChangeForm::SetAngleY),
      make_column("angle_z", &SqliteChangeForm::GetAngleZ,
                  &SqliteChangeForm::SetAngleZ),
      make_column("inventory_dump", &SqliteChangeForm::GetInventory,
                  &SqliteChangeForm::SetInventory),
      make_column("is_harvested", &SqliteChangeForm::isHarvested),
      make_column("is_open", &SqliteChangeForm::isOpen),
      make_column("next_reloot_datetime",
                  &SqliteChangeForm::nextRelootDatetime),
      make_column("world_or_cell", &SqliteChangeForm::worldOrCell),
      make_column("is_race_menu_open", &SqliteChangeForm::isRaceMenuOpen),
      make_column("look_dump", &SqliteChangeForm::GetLook,
                  &SqliteChangeForm::SetLook),
      make_column("equipment_dump", &SqliteChangeForm::GetEquipment,
                  &SqliteChangeForm::SetEquipment)));
  storage.sync_schema(true);
  return storage;
}

using SqliteStorage = decltype(MakeSqliteStorage(""));
}

struct SqliteSaveStorage::Impl
{
  struct
  {
    std::unique_ptr<SqliteStorage> storage;
    std::mutex m;
  } share;

  struct
  {
    std::list<std::exception_ptr> exceptions;
    std::mutex m;
  } share2;

  struct
  {

  } share3;

  std::unique_ptr<std::thread> thr;
  std::atomic<bool> destroyed = false;
};

SqliteSaveStorage::SqliteSaveStorage(const char* filename)
  : pImpl(new Impl, [](Impl* p) { delete p; })
{
  pImpl->share.storage.reset(new SqliteStorage(MakeSqliteStorage(filename)));

  auto p = this->pImpl.get();
  pImpl->thr.reset(new std::thread([p] { SaverThreadMain(p); }));
}

SqliteSaveStorage::~SqliteSaveStorage()
{
  pImpl->destroyed = true;
  pImpl->thr->join();
}

void SqliteSaveStorage::SaverThreadMain(Impl* pImpl)
{
  while (!pImpl->destroyed) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    try {

    } catch (...) {
      std::lock_guard l(pImpl->share2.m);
      auto exceptionPtr = std::current_exception();
      pImpl->share2.exceptions.push_back(exceptionPtr);
    }
  }
}

void SqliteSaveStorage::IterateSync(const IterateSyncCallback& cb)
{
  std::lock_guard l(pImpl->share.m);
  for (auto& changeForm : pImpl->share.storage->iterate<SqliteChangeForm>())
    cb(changeForm);
}

void SqliteSaveStorage::Upsert(const std::vector<MpChangeForm>& changeForms,
                               const UpsertCallback& cb)
{
  std::lock_guard l(pImpl->share.m);
}

void SqliteSaveStorage::Tick()
{
  {
    std::lock_guard l(pImpl->share2.m);
    if (!pImpl->share2.exceptions.empty()) {
      auto exceptionPtr = std::move(pImpl->share2.exceptions.front());
      pImpl->share2.exceptions.pop_front();
      std::rethrow_exception(exceptionPtr);
    }
  }
}