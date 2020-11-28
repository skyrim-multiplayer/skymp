#include "SqliteSaveStorage.h"
#include "SqliteChangeForm.h"
#include <atomic>
#include <list>
#include <mutex>
#include <sqlite_orm.h>
#include <thread>

using namespace sqlite_orm;

#define MAKE_STORAGE(name)                                                    \
  auto storage = make_storage(                                                \
    name,                                                                     \
    make_table<SqliteChangeForm>(                                             \
      "SqliteChangeForm",                                                     \
      make_column("primary", &SqliteChangeForm::primary, autoincrement(),     \
                  primary_key()),                                             \
      make_column("record_type", &SqliteChangeForm::recType),                 \
      make_column("base_desc", &SqliteChangeForm::GetBaseFormDesc,            \
                  &SqliteChangeForm::SetBaseFormDesc),                        \
      make_column("form_desc", &SqliteChangeForm::GetFormDesc,                \
                  &SqliteChangeForm::SetFormDesc),                            \
      make_column("x", &SqliteChangeForm::GetX, &SqliteChangeForm::SetX),     \
      make_column("y", &SqliteChangeForm::GetY, &SqliteChangeForm::SetY),     \
      make_column("z", &SqliteChangeForm::GetZ, &SqliteChangeForm::SetZ),     \
      make_column("angle_x", &SqliteChangeForm::GetAngleX,                    \
                  &SqliteChangeForm::SetAngleX),                              \
      make_column("angle_y", &SqliteChangeForm::GetAngleY,                    \
                  &SqliteChangeForm::SetAngleY),                              \
      make_column("angle_z", &SqliteChangeForm::GetAngleZ,                    \
                  &SqliteChangeForm::SetAngleZ),                              \
      make_column("json_data", &SqliteChangeForm::GetJsonData,                \
                  &SqliteChangeForm::SetJsonData),                            \
      make_column("is_harvested", &SqliteChangeForm::isHarvested),            \
      make_column("is_open", &SqliteChangeForm::isOpen),                      \
      make_column("next_reloot_datetime",                                     \
                  &SqliteChangeForm::nextRelootDatetime),                     \
      make_column("world_or_cell", &SqliteChangeForm::worldOrCell),           \
      make_column("is_race_menu_open", &SqliteChangeForm::isRaceMenuOpen),    \
      make_column("base_container_added",                                     \
                  &SqliteChangeForm::baseContainerAdded)));

namespace {
class SqliteDbImpl : public IDatabase
{
public:
  SqliteDbImpl(std::string filename_)
    : filename(filename_)
  {
    MAKE_STORAGE(filename.data());

    auto res = storage.sync_schema_simulate(true);

    std::vector<std::string> destructiveActions;

    for (auto [str, result] : res) {
      const char* action = "";
      switch (result) {
        case sync_schema_result::dropped_and_recreated:
          action = "dropped_and_recreated";
          break;
        case sync_schema_result::new_columns_added_and_old_columns_removed:
          action = "new_columns_added_and_old_columns_removed";
          break;
        case sync_schema_result::old_columns_removed:
          action = "old_columns_removed";
          break;
      }
      if (action[0])
        destructiveActions.push_back(action + (" (target is " + str + ")"));
    }
    if (destructiveActions.size()) {
      std::stringstream ss;
      ss << "Sqlite is going to take some destructive actions: ";
      for (auto v : destructiveActions)
        ss << v << "; ";
      throw std::runtime_error(ss.str());
    }
    storage.sync_schema(true);
  }

  size_t Upsert(const std::vector<MpChangeForm>& changeForms) override
  {
    MAKE_STORAGE(filename.data());

    auto g = storage.transaction_guard();
    int numChangeForms = 0;
    auto was = clock();
    std::map<FormDesc, int> existingFormDescs;
    for (auto changeForm : storage.iterate<SqliteChangeForm>()) {
      existingFormDescs.insert({ changeForm.formDesc, changeForm.primary });
    }

    std::vector<SqliteChangeForm> toInsert, toUpdate;

    for (auto& changeForm : changeForms) {
      SqliteChangeForm f;
      std::vector<SqliteChangeForm>* target;

      if (auto it = existingFormDescs.find(changeForm.formDesc);
          it != existingFormDescs.end()) {
        f.primary = it->second;
        target = &toUpdate;
      } else {
        f.primary = -1;
        target = &toInsert;
      }
      numChangeForms++;

      static_cast<MpChangeForm&>(f) = std::move(changeForm);
      target->push_back(f);
    }

    storage.insert_range(toInsert.data(), toInsert.data() + toInsert.size());
    for (auto& v : toUpdate)
      storage.update(v);
    g.commit();
    return toUpdate.size() + toInsert.size();
  }

  void Iterate(const IterateCallback& iterateCallback) override
  {
    MAKE_STORAGE(filename.data());
    for (auto v : storage.iterate<SqliteChangeForm>())
      iterateCallback(v);
  }

private:
  const std::string filename;
};
}

SqliteSaveStorage::SqliteSaveStorage(std::string filename,
                                     std::shared_ptr<spdlog::logger> logger)
  : AsyncSaveStorage(CreateDbImpl(filename), logger)
{
}

std::shared_ptr<IDatabase> SqliteSaveStorage::CreateDbImpl(
  std::string filename)
{
  return std::shared_ptr<SqliteDbImpl>(new SqliteDbImpl(filename));
}
