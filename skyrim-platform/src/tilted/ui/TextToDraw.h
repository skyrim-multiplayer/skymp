#pragma once

#include <DirectXTK/SpriteBatch.h>
#include <array>
#include <functional>
#include <string>
struct TextToDraw
{
  std::wstring fontName;
  double x = 0.f;
  double y = 0.f;
  std::wstring string;
  std::array<double, 4> color = { 0.f, 0.f, 1.f, 1.f };
  float rotation = 0.f;
  float size = 1;
  DirectX::SpriteEffects effects = DirectX::SpriteEffects_None;
  int layerDepth = 0;
  std::array<double, 2> origin = { 0.f, 0.f };

  // Original screen position set by the user (preserved across attach/detach)
  double savedX = 0.f;
  double savedY = 0.f;

  // Reference attachment: when refrFormId != 0, position is driven each frame
  // by the refr's screen projection.
  uint32_t refrFormId = 0;
  std::string refrNodeName;
  std::array<double, 3> refrOffset = { 0.0, 0.0, 0.0 };
  std::array<double, 2> screenOffset = { 0.0, 0.0 };
  bool refrDirty = false; // true until OnUpdate computes the screen position
};

using TextToDrawCallback = std::function<void(const TextToDraw& textToDraw)>;
using ObtainTextsToDrawFunction =
  std::function<void(TextToDrawCallback callback)>;
