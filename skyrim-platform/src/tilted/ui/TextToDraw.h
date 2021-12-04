#pragma once
#include <array>
#include <string>

struct TextToDraw
{
  float x = 0.f;
  float y = 0.f;
  std::wstring kString;
  std::array<float, 4> color = { 0.f, 0.f, 1.f, 1.f };
};
