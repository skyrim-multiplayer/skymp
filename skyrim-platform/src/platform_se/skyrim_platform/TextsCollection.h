#pragma once

#include <string>
#include "ui/TextToDraw.h"
#include "JsEngine.h"

class TextsCollection
{
public:
  ~TextsCollection();

  int CreateText(float xPos, float yPos, std::wstring str,
                  std::array<double, 4> color);

  void DestroyText(int textId);

  void SetTextPos(int textId, float xPos, float yPos);

  void SetTextString(int textId, std::wstring str);

  void SetTextColor(int textId, std::array<double, 4> color);

  void DestroyAllTexts();

  static TextsCollection& GetSinglton() noexcept;

public:
  TextsCollection(const TextsCollection&) = delete;
  TextsCollection(TextsCollection&&) = delete;

  TextsCollection& operator=(const TextsCollection&) = delete;
  TextsCollection& operator=(const TextsCollection&&) = delete;

public:
  std::pair<float, float> GetTextPos(int textId) const;

  std::wstring GetTextString(int textId) const;

  std::array<double, 4> GetTextColor(int textId) const;

  const std::unordered_map<int, TextToDraw>& GetCreatedTexts() const;

  int GetTextCount() const noexcept { return textCount; }

private:
  TextsCollection();

  std::unordered_map<int, TextToDraw> texts;
  uint32_t textCount;
};