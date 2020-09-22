#pragma once
#include "MpChangeForms.h"
#include <string>

class SqliteChangeForm : public MpChangeForm
{
public:
  int primary = 0;

  std::string GetInventory() const;
  void SetInventory(const std::string& inventoryDump);

  std::string GetEquipment() const;
  void SetEquipment(const std::string& equipmentDump);

  std::string GetFormDesc() const;
  void SetFormDesc(const std::string& formDesc);

  std::string GetBaseFormDesc() const;
  void SetBaseFormDesc(const std::string& formDesc);

  std::string GetLook() const;
  void SetLook(const std::string& lookDump);

  float GetX() const;
  void SetX(float v);
  float GetY() const;
  void SetY(float v);
  float GetZ() const;
  void SetZ(float v);

  float GetAngleX() const;
  void SetAngleX(float v);
  float GetAngleY() const;
  void SetAngleY(float v);
  float GetAngleZ() const;
  void SetAngleZ(float v);
};