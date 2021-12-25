#include "TextsCollection.h"

TextsCollection::TextsCollection()
  : textCount(0)
{	}

TextsCollection::~TextsCollection()
{  }

int TextsCollection::CreateText(float xPos, float yPos, std::wstring str,
                                 std::array<double, 4> color = { 0.f, 0.f, 1.f,
                                                                 1.f })
{
  TextToDraw text{ xPos, yPos, str, color };
  textCount++;
 
  std::pair<uint32_t, TextToDraw> arg = { textCount, text };
  texts.insert(arg);

  return textCount;
}

void TextsCollection::DestroyText(int textId)
{
  texts.erase(textId);
}

int TextsCollection::SetTextPos(uint32_t textId, float xPos, float yPos)
{
  texts.at(textId).x = xPos;
  texts.at(textId).y = yPos;
  return textId;
}

int TextsCollection::SetTextString(uint32_t textId, std::wstring str)
{
  texts.at(textId).string = str;
  return textId;
}

int TextsCollection::SetTextColor(uint32_t textId,
                                   std::array<double, 4> color)
{
  texts.at(textId).color = color;
  return textId;
}

void TextsCollection::DestroyAllTexts()
{
  texts.clear();
  textCount = 0;
}

TextsCollection& TextsCollection::GetSinglton() noexcept
{
  static TextsCollection text;

  return text;
}

//Getters

std::pair<float, float> TextsCollection::GetTextPos(
  uint32_t textId) const 
{  
    std::pair<float, float> positions = {
    texts.at(textId).x,
    texts.at(textId).y,
  };

  return positions;
}

std::wstring TextsCollection::GetTextString(uint32_t textId) const 
{
  return texts.at(textId).string;
}

std::array<double, 4> TextsCollection::GetTextColor(
  uint32_t textId) const 
{
  return texts.at(textId).color;
}

const std::unordered_map<int, TextToDraw>&
TextsCollection::GetCreatedTexts() const
{
  return texts;
}