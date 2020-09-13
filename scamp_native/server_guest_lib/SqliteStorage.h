#pragma once
#include "MpChangeForms.h"
#include <sqlite_orm/sqlite_orm.h>

inline auto MakeSqliteStorage(const char* name)
{
  using namespace sqlite_orm;

  auto storage = make_storage(
    name,
    make_table<MpChangeForm>(
      "MpChangeForm", make_column("record_type", &MpChangeForm::recType),
      make_column("base_desc", &MpChangeForm::GetBaseFormDesc,
                  &MpChangeForm::SetBaseFormDesc),
      make_column("form_desc", &MpChangeForm::GetFormDesc,
                  &MpChangeForm::SetFormDesc, primary_key()),
      make_column("x", &MpChangeForm::GetX, &MpChangeForm::SetX),
      make_column("y", &MpChangeForm::GetY, &MpChangeForm::SetY),
      make_column("z", &MpChangeForm::GetZ, &MpChangeForm::SetZ),
      make_column("angle_x", &MpChangeForm::GetAngleX,
                  &MpChangeForm::SetAngleX),
      make_column("angle_y", &MpChangeForm::GetAngleY,
                  &MpChangeForm::SetAngleY),
      make_column("angle_z", &MpChangeForm::GetAngleZ,
                  &MpChangeForm::SetAngleZ),
      make_column("inventory_dump", &MpChangeForm::GetInventory,
                  &MpChangeForm::SetInventory),
      make_column("is_harvested", &MpChangeForm::isHarvested),
      make_column("is_open", &MpChangeForm::isOpen),
      make_column("next_reloot_datetime", &MpChangeForm::nextRelootDatetime),
      make_column("world_or_cell", &MpChangeForm::worldOrCell),
      make_column("is_race_menu_open", &MpChangeForm::isRaceMenuOpen),
      make_column("look_dump", &MpChangeForm::GetLook, &MpChangeForm::SetLook),
      make_column("equipment_dump", &MpChangeForm::GetEquipment,
                  &MpChangeForm::SetEquipment)));
  return storage;
}