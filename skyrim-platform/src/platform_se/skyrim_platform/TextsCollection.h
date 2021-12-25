#pragma once

#include <string>
#include <vector>
#include "ui/TextToDraw.h"
#include "JsEngine.h"

class TextsCollection
{
public:
  ~TextsCollection();

  int CreateText(float xPos, float yPos, std::wstring str,
                  std::array<double, 4> color);

  void DestroyText(int textId);

  int SetTextPos(uint32_t textId, float xPos, float yPos);

  int SetTextString(uint32_t textId, std::wstring str);

  int SetTextColor(uint32_t textId, std::array<double, 4> color);

  void DestroyAllTexts();

  static TextsCollection& GetSinglton() noexcept;

public:
  TextsCollection(const TextsCollection&) = delete;
  TextsCollection(TextsCollection&&) = delete;

  TextsCollection& operator=(const TextsCollection&) = delete;
  TextsCollection& operator=(const TextsCollection&&) = delete;

public:
  std::pair<float, float> GetTextPos(uint32_t textId) const;

  std::wstring GetTextString(uint32_t textId) const;

  std::array<double, 4> GetTextColor(uint32_t textId) const;

  const std::unordered_map<int, TextToDraw>& GetCreatedTexts() const;

  int GetTextCount() const noexcept { return textCount; }

private:
  TextsCollection();

  std::unordered_map<int, TextToDraw> texts;
  //std::vector<uint32_t> deleted_texts;
  uint32_t textCount;
};