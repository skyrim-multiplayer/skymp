#include "SqliteChangeForm.h"
#include "MpActor.h"
#include "WorldState.h"
#include <ctime>
#include <simdjson.h>

namespace {
template <class T>
T DumpToStruct(const std::string& dump)
{
  if (dump.size() > 0) {
    simdjson::dom::parser p;
    auto element = p.parse(dump).value();
    return T::FromJson(element);
  }
  return T();
}
}

std::string SqliteChangeForm::GetInventory() const
{
  return inv.ToJson().dump();
}

void SqliteChangeForm::SetInventory(const std::string& inventoryDump)
{
  inv = DumpToStruct<Inventory>(inventoryDump);
}

std::string SqliteChangeForm::GetEquipment() const
{
  if (equipment)
    return equipment->ToJson().dump();
  else
    return "";
}

void SqliteChangeForm::SetEquipment(const std::string& equipmentDump)
{
  if (equipmentDump.size() > 0)
    equipment = DumpToStruct<Equipment>(equipmentDump);
  else
    equipment.reset();
}

std::string SqliteChangeForm::GetFormDesc() const
{
  return formDesc.ToString();
}

void SqliteChangeForm::SetFormDesc(const std::string& newFormDesc)
{
  formDesc = FormDesc::FromString(newFormDesc);
}

std::string SqliteChangeForm::GetBaseFormDesc() const
{
  return baseDesc.ToString();
}

void SqliteChangeForm::SetBaseFormDesc(const std::string& newFormDesc)
{
  baseDesc = FormDesc::FromString(newFormDesc);
}

std::string SqliteChangeForm::GetLook() const
{
  if (look)
    return look->ToJson();
  else
    return "";
}

void SqliteChangeForm::SetLook(const std::string& lookDump)
{
  if (lookDump.size() > 0)
    look = DumpToStruct<Look>(lookDump);
  else
    look.reset();
}

float SqliteChangeForm::GetX() const
{
  return position.x;
}

void SqliteChangeForm::SetX(float v)
{
  position.x = v;
}

float SqliteChangeForm::GetY() const
{
  return position.y;
}

void SqliteChangeForm::SetY(float v)
{
  position.y = v;
}

float SqliteChangeForm::GetZ() const
{
  return position.z;
}

void SqliteChangeForm::SetZ(float v)
{
  position.z = v;
}

float SqliteChangeForm::GetAngleX() const
{
  return angle.x;
}

void SqliteChangeForm::SetAngleX(float v)
{
  angle.x = v;
}

float SqliteChangeForm::GetAngleY() const
{
  return angle.y;
}

void SqliteChangeForm::SetAngleY(float v)
{
  angle.y = v;
}

float SqliteChangeForm::GetAngleZ() const
{
  return angle.z;
}

void SqliteChangeForm::SetAngleZ(float v)
{
  angle.z = v;
}