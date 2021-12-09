#pragma once

#include <array>
#include <functional>
#include <string>

struct TextToDraw
{
  float x = 0.f;
  float y = 0.f;
  std::wstring string;
  std::array<float, 4> color = { 0.f, 0.f, 1.f, 1.f };
};

using TextToDrawCallback = std::function<void(const TextToDraw& textToDraw)>;
using ObtainTextsToDrawFunction =
  std::function<void(TextToDrawCallback callback)>;
