#pragma once

#include <DirectXTK/SpriteBatch.h>
#include <array>
#include <functional>
#include <string>
struct TextToDraw
{
  std::wstring fontName = L"Tavern";
  double x = 0.f;
  double y = 0.f;
  std::wstring string;
  std::array<double, 4> color = { 0.f, 0.f, 1.f, 1.f };
  float rotation = 0.f;
  float size = 1;
  DirectX::SpriteEffects effects = DirectX::SpriteEffects_None;
  float layerDepth = 0.f;
  std::array<double, 2> origin = { 0.f, 0.f };
};

using TextToDrawCallback = std::function<void(const TextToDraw& textToDraw)>;
using ObtainTextsToDrawFunction =
  std::function<void(TextToDrawCallback callback)>;
