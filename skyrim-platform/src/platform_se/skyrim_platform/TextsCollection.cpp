#include "TextsCollection.h"

TextsCollection::TextsCollection()
{
  makeId.reset(new MakeID(std::numeric_limits<uint32_t>::max()));
}

TextsCollection::~TextsCollection()
{
}

int TextsCollection::CreateText(double xPos, double yPos, std::wstring str,
                                std::array<double, 4> color = { 0.f, 0.f, 1.f,
                                                                1.f },
                                std::wstring name = L"Tavern")
{
  TextToDraw text{ name, xPos, yPos, str, color };

  uint32_t id;
  makeId->CreateID(id);

  if (texts.size() <= id) {
    texts.resize(id);
  }

  texts[id] = text;
  return id;
}

void TextsCollection::DestroyText(int textId)
{
  if (makeId->IsID(textId) == false)
    return;
  texts.erase(texts.begin() + textId);
  makeId->DestroyID(textId);
}
void TextsCollection::DestroyAllTexts()
{
  texts.clear();
  makeId.reset(new MakeID(std::numeric_limits<uint32_t>::max()));
}

void TextsCollection::SetTextPos(int& textId, double& xPos, double& yPos)
{
  if (makeId->IsID(textId) == false)
    return;
  texts.at(textId).x = xPos;
  texts.at(textId).y = yPos;
}
void TextsCollection::SetTextString(int& textId, std::wstring& str)
{
  if (makeId->IsID(textId) == false)
    return;
  texts.at(textId).string = std::move(str);
}
void TextsCollection::SetTextColor(int& textId, std::array<double, 4>& color)
{
  if (makeId->IsID(textId) == false)
    return;
  texts.at(textId).color = color;
}
void TextsCollection::SetTextFont(int& textId, std::wstring& name)
{
  if (makeId->IsID(textId) == false)
    return;
  texts.at(textId).fontName = name;
}
void TextsCollection::SetTextRotation(int& textId, float& rotation)
{
  if (makeId->IsID(textId) == false)
    return;
  texts.at(textId).rotation = rotation;
}
void TextsCollection::SetTextSize(int& textId, float& size)
{
  if (makeId->IsID(textId) == false)
    return;
  texts.at(textId).size = size;
}
void TextsCollection::SetTextEffect(int& textId, int& effect)
{
  if (makeId->IsID(textId) == false)
    return;
  texts.at(textId).effects = static_cast<DirectX::SpriteEffects>(effect);
}
void TextsCollection::SetTextDepth(int& textId, int& depth)
{
  if (makeId->IsID(textId) == false)
    return;
  texts.at(textId).layerDepth = depth;
}
void TextsCollection::SetTextOrigin(int& textId, std::array<double, 2>& origin)
{
  if (makeId->IsID(textId) == false)
    return;
  texts.at(textId).origin = origin;
}

const bool TextsCollection::GetTextPos(int textId,
                                       std::pair<double, double>& result) const
{
  if (makeId->IsID(textId) == false) {
    return false;
  }

  std::pair<double, double> positions = {
    texts.at(textId).x,
    texts.at(textId).y,
  };

  result = positions;
  return true;
}
const bool TextsCollection::GetTextString(int textId,
                                          std::wstring& result) const
{
  if (makeId->IsID(textId) == false) {
    false;
  }
  result = texts.at(textId).string;
  return true;
}
const bool TextsCollection::GetTextColor(int textId,
                                         std::array<double, 4>& result) const
{
  if (makeId->IsID(textId) == false) {
    return false;
  }
  result = texts.at(textId).color;
  return true;
}
const bool TextsCollection::GetTextFont(int textId, std::wstring& result) const
{
  if (makeId->IsID(textId) == false) {
    return false;
  }
  result = texts.at(textId).fontName;
  return true;
}
const bool TextsCollection::GetTextRotation(int textId, float& result) const
{
  if (makeId->IsID(textId) == false) {
    return false;
  }
  result = texts.at(textId).rotation;
  return true;
}
const bool TextsCollection::GetTextSize(int textId, float& result) const
{
  if (makeId->IsID(textId) == false) {
    return false;
  }
  result = texts.at(textId).size;
  return true;
}
const bool TextsCollection::GetTextEffect(int textId, int& result) const
{
  if (makeId->IsID(textId) == false) {
    return false;
  }
  result = texts.at(textId).effects;
  return true;
}
const bool TextsCollection::GetTextDepth(int textId, int& result) const
{
  if (makeId->IsID(textId) == false) {
    return false;
  }
  result = texts.at(textId).layerDepth;
  return true;
}
const bool TextsCollection::GetTextOrigin(int textId,
                                          std::array<double, 2>& result) const
{
  if (makeId->IsID(textId) == false) {
    return false;
  }
  result = texts.at(textId).origin;
  return true;
}

const std::vector<TextToDraw>& TextsCollection::GetCreatedTexts() const
{
  return texts;
}

TextsCollection& TextsCollection::GetSingleton() noexcept
{
  static TextsCollection text;
  return text;
}
