#pragma once
#include <array>
#include <string>

struct TextToDraw
{
  TextToDraw(float x_, float y_, wchar_t* m_kString);

  float x;
  float y;
  std::wstring kString;
  std::array<float, 4> color = { 0.f, 0.f, 1.f, 1.f };
};
