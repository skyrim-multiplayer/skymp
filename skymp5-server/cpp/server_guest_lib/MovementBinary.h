#pragma once

#include <string_view>

class MovementBinary
{
public:
  size_t GetIdx() const;

  float GetX() const;
  float GetY() const;
  float GetZ() const;

  void SetX(float value);
  // ...

private:
  // std::string_view data;
  char* data;
};
