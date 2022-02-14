#include "TextsCollection.h"

TextsCollection::TextsCollection()
  : textCount(0)
{
}

TextsCollection::~TextsCollection()
{
}

int TextsCollection::CreateText(double xPos, double yPos, std::wstring str,
                                std::array<double, 4> color = { 0.f, 0.f, 1.f,
                                                                1.f })
{
  TextToDraw text{ xPos, yPos, std::move(str), std::move(color) };

  textCount++;
  std::pair<int, TextToDraw> arg = { textCount, text };

  texts.insert(arg);

  return textCount;
}

void TextsCollection::DestroyText(int textId)
{
  texts.erase(textId);
}

void TextsCollection::SetTextPos(int textId, float xPos, float yPos)
{
  texts.at(textId).x = xPos;
  texts.at(textId).y = yPos;
}

void TextsCollection::SetTextString(int textId, std::wstring str)
{
  texts.at(textId).string = std::move(str);
}

void TextsCollection::SetTextColor(int textId, std::array<double, 4> color)
{
  texts.at(textId).color = color;
}

void TextsCollection::DestroyAllTexts()
{
  texts.clear();
  textCount = 0;
}

TextsCollection& TextsCollection::GetSingleton() noexcept
{
  static TextsCollection text;
  return text;
}

std::pair<double, double> TextsCollection::GetTextPos(int textId) const
{
  std::pair<double, double> positions = {
    texts.at(textId).x,
    texts.at(textId).y,
  };

  return positions;
}

const std::wstring& TextsCollection::GetTextString(int textId) const
{
  return texts.at(textId).string;
}

std::array<double, 4> TextsCollection::GetTextColor(int textId) const
{
  return texts.at(textId).color;
}

const std::unordered_map<int, TextToDraw>& TextsCollection::GetCreatedTexts()
  const
{
  return texts;
}
