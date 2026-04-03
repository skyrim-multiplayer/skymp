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
                                                                1.f },
                                std::wstring name = L"Tavern")
{
  TextToDraw text{ name, xPos, yPos, str, color };
  text.savedX = xPos;
  text.savedY = yPos;

  textCount++;
  std::pair<int, TextToDraw> arg = { textCount, text };

  texts.insert(arg);

  return textCount;
}

void TextsCollection::DestroyText(int textId)
{
  texts.erase(textId);
}
void TextsCollection::DestroyAllTexts()
{
  texts.clear();
  textCount = 0;
}

void TextsCollection::SetTextPos(int& textId, double& xPos, double& yPos)
{
  auto& text = texts.at(textId);
  text.x = xPos;
  text.y = yPos;
  text.savedX = xPos;
  text.savedY = yPos;
}
void TextsCollection::SetTextString(int& textId, std::wstring& str)
{
  texts.at(textId).string = std::move(str);
}
void TextsCollection::SetTextColor(int& textId, std::array<double, 4>& color)
{
  texts.at(textId).color = color;
}
void TextsCollection::SetTextFont(int& textId, std::wstring& name)
{
  texts.at(textId).fontName = name;
}
void TextsCollection::SetTextRotation(int& textId, float& rotation)
{
  texts.at(textId).rotation = rotation;
}
void TextsCollection::SetTextSize(int& textId, float& size)
{
  texts.at(textId).size = size;
}
void TextsCollection::SetTextEffect(int& textId, int& effect)
{
  texts.at(textId).effects = static_cast<DirectX::SpriteEffects>(effect);
}
void TextsCollection::SetTextDepth(int& textId, int& depth)
{
  texts.at(textId).layerDepth = depth;
}
void TextsCollection::SetTextOrigin(int& textId, std::array<double, 2>& origin)
{
  texts.at(textId).origin = origin;
}
void TextsCollection::SetTextRefr(int& textId, uint32_t& refrFormId)
{
  auto& text = texts.at(textId);
  // Detaching: restore the original user-set screen position
  if (refrFormId == 0 && text.refrFormId != 0) {
    text.x = text.savedX;
    text.y = text.savedY;
  }
  text.refrFormId = refrFormId;
  if (refrFormId != 0) {
    text.refrDirty = true;
  }
}
void TextsCollection::SetTextRefrNode(int& textId, std::string& nodeName)
{
  texts.at(textId).refrNodeName = std::move(nodeName);
}
void TextsCollection::SetTextRefrOffset(int& textId,
                                        std::array<double, 3>& offset)
{
  texts.at(textId).refrOffset = offset;
}

void TextsCollection::SetTextRefrScreenOffset(int& textId,
                                              std::array<double, 2>& offset)
{
  texts.at(textId).screenOffset = offset;
}

const std::pair<double, double> TextsCollection::GetTextPos(int textId) const
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
const std::array<double, 4>& TextsCollection::GetTextColor(int textId) const
{
  return texts.at(textId).color;
}
const std::wstring& TextsCollection::GetTextFont(int textId) const
{
  return texts.at(textId).fontName;
}
const float& TextsCollection::GetTextRotation(int textId) const
{
  return texts.at(textId).rotation;
}
const float& TextsCollection::GetTextSize(int textId) const
{
  return texts.at(textId).size;
}
const int TextsCollection::GetTextEffect(int textId) const
{
  return texts.at(textId).effects;
}
const int& TextsCollection::GetTextDepth(int textId) const
{
  return texts.at(textId).layerDepth;
}
const std::array<double, 2> TextsCollection::GetTextOrigin(int textId) const
{
  return texts.at(textId).origin;
}

uint32_t TextsCollection::GetTextRefr(int textId) const
{
  return texts.at(textId).refrFormId;
}

const std::string& TextsCollection::GetTextRefrNode(int textId) const
{
  return texts.at(textId).refrNodeName;
}

const std::array<double, 3>& TextsCollection::GetTextRefrOffset(
  int textId) const
{
  return texts.at(textId).refrOffset;
}
const std::array<double, 2>& TextsCollection::GetTextRefrScreenOffset(
  int textId) const
{
  return texts.at(textId).screenOffset;
}

const std::unordered_map<int, TextToDraw>& TextsCollection::GetCreatedTexts()
  const
{
  return texts;
}

std::unordered_map<int, TextToDraw>& TextsCollection::GetCreatedTexts()
{
  return texts;
}

TextsCollection& TextsCollection::GetSingleton() noexcept
{
  static TextsCollection text;
  return text;
}
