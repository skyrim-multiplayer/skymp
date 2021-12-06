#pragma once

#include <array>
#include <string>

struct TextToDraw
{
  float x = 0.f;
  float y = 0.f;
  std::wstring string;
  std::array<float, 4> color = { 255.f, 255.f, 255.f, 1.f };
};
