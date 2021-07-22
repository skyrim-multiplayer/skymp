#pragma once
#include "MpChangeForms.h"
#include <string>

class SqliteChangeForm : public MpChangeForm
{
public:
  int primary = 0;

  std::string GetJsonData() const;
  void SetJsonData(const std::string& jsonData);

  std::string GetFormDesc() const;
  void SetFormDesc(const std::string& formDesc);

  std::string GetBaseFormDesc() const;
  void SetBaseFormDesc(const std::string& formDesc);

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
