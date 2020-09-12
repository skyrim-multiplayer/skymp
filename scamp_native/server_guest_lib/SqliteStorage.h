#pragma once
#include "MpChangeForms.h"
#include <sqlite_orm/sqlite_orm.h>

inline auto MakeSqliteStorage(const char* name)
{
  using namespace sqlite_orm;

  auto storage = make_storage(
    name,
    make_table(
      "MpChangeForm",
      make_column("id", &MpChangeForm::id, autoincrement(), primary_key()),
      make_column("short_form_id", &MpChangeForm::shortFormId),
      make_column("file", &MpChangeForm::file),
      make_column("pos", &MpChangeForm::pos),
      make_column("rot", &MpChangeForm::rot),
      make_column("inventoryDump", &MpChangeForm::GetInventoryDump,
                  &MpChangeForm::SetInventoryDump),
      make_column("is_harvested", &MpChangeForm::isHarvested),
      make_column("is_open", &MpChangeForm::isOpen),
      make_column("next_reloot_datetime", &MpChangeForm::nextRelootDatetime),
      make_column("world_or_cell", &MpChangeForm::worldOrCell),
      make_column("is_race_menu_open", &MpChangeForm::isRaceMenuOpen),
      make_column("look_dump", &MpChangeForm::lookDump),
      make_column("equipment_dump", &MpChangeForm::equipmentDump)));
  return storage;
}