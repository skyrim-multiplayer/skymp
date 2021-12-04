#pragma once

struct TextToDraw
{
  TextToDraw(float x_, float y_, wchar_t* m_kString);

  float x;
  float y;
  float size;
  wchar_t* kString;
};
