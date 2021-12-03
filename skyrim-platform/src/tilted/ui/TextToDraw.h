#pragma once

struct TextToDraw
{
  TextToDraw(wchar_t* m_kString);

  float x;
  float y;
  float size;
  wchar_t* kString;
  DirectX::XMVECTORF32 color;
};
