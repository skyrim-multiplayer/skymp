#pragma once

#include <DirectXTK/SpriteBatch.h>
#include <MakeID.h-1.0.2>

class TextsCollection
{
public:
  ~TextsCollection();

  int CreateText(double xPos, double yPos, std::wstring string,
                 std::array<double, 4> color, std::wstring name);

  void DestroyText(int textId);
  void DestroyAllTexts();

  void SetTextPos(int& textId, double& xPos, double& yPos);
  void SetTextString(int& textId, std::wstring& str);
  void SetTextColor(int& textId, std::array<double, 4>& color);
  void SetTextFont(int& textId, std::wstring& name);
  void SetTextRotation(int& textId, float& rotation);
  void SetTextSize(int& textId, float& size);
  void SetTextEffect(int& textId, int& effect);
  void SetTextDepth(int& textId, int& depth);
  void SetTextOrigin(int& textId, std::array<double, 2>& origin);

  static TextsCollection& GetSingleton() noexcept;

public:
  TextsCollection(const TextsCollection&) = delete;
  TextsCollection(TextsCollection&&) = delete;

  TextsCollection& operator=(const TextsCollection&) = delete;
  TextsCollection& operator=(const TextsCollection&&) = delete;

public:
  const bool GetTextPos(int textId, std::pair<double, double>& result) const;
  const bool GetTextString(int textId, std::wstring& result) const;
  const bool GetTextColor(int textId, std::array<double, 4>& result) const;
  const bool GetTextFont(int textId, std::wstring& result) const;
  const bool GetTextRotation(int textId, float& result) const;
  const bool GetTextSize(int textId, float& result) const;
  const bool GetTextEffect(int textId, int& result) const;
  const bool GetTextDepth(int textId, int& result) const;
  const bool GetTextOrigin(int textId, std::array<double, 2>& result) const;

  const std::vector<TextToDraw>& GetCreatedTexts() const;

  int GetNumCreatedTexts() const noexcept { return texts.size(); }

private:
  TextsCollection();

private:
  std::unique_ptr<MakeID> makeId;
  std::vector<TextToDraw> texts;
};
