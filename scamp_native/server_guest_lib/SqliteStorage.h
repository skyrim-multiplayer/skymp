#pragma once
#include "MpChangeForms.h"
#include <sqlite_orm/sqlite_orm.h>

inline auto MakeSqliteStorage(const char* name)
{
  using namespace sqlite_orm;

  auto storage = make_storage(name,
                              make_table("MpChangeForm",
                                         make_column("record_type",
                                                     &MpChangeForm::
                                                       recType) /*,
make_column("base_desc", &MpChangeForm::GetBaseFormDesc,
          &MpChangeForm::SetBaseFormDesc),
make_column("form_desc", &MpChangeForm::GetFormDesc,
          &MpChangeForm::SetFormDesc, primary_key()),
make_column("pos", &MpChangeForm::pos),
make_column("rot", &MpChangeForm::rot),
make_column("inventory_dump", &MpChangeForm::GetInventory,
          &MpChangeForm::SetInventory),
make_column("is_harvested", &MpChangeForm::isHarvested),
make_column("is_open", &MpChangeForm::isOpen),
make_column("next_reloot_datetime", &MpChangeForm::nextRelootDatetime),
make_column("world_or_cell", &MpChangeForm::worldOrCell),
make_column("is_race_menu_open", &MpChangeForm::isRaceMenuOpen),
make_column("look_dump", &MpChangeForm::GetLook, &MpChangeForm::SetLook),
make_column("equipment_dump", &MpChangeForm::GetEquipment,
          &MpChangeForm::SetEquipment)*/));
  return storage;
}