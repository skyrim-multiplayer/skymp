#pragma once

#include <DirectXTK/SpriteBatch.h>

class TextsCollection
{
public:
  ~TextsCollection();

  int CreateText(double xPos, double yPos, std::wstring string,
                 std::array<double, 4> color, std::wstring name);

  void DestroyText(int textId);

  void SetTextPos(int textId, float xPos, float yPos);

  void SetTextString(int textId, std::wstring str);

  void SetTextColor(int textId, std::array<double, 4> color);
  void SetTextFont(int textId, std::wstring name);
  void SetTextRotation(int textId, float rotation);

  void SetTextSize(int textId, float size);

  void SetTextEffect(int textId, int effect);

  void SetTextDepth(int textId, float depth);

  void SetTextOrigin(int textId, std::array<double, 2> origin);


  void DestroyAllTexts();

  static TextsCollection& GetSingleton() noexcept;

public:
  TextsCollection(const TextsCollection&) = delete;
  TextsCollection(TextsCollection&&) = delete;

  TextsCollection& operator=(const TextsCollection&) = delete;
  TextsCollection& operator=(const TextsCollection&&) = delete;

public:
  std::pair<double, double> GetTextPos(int textId) const;

  const std::wstring& GetTextString(int textId) const;

  std::array<double, 4> GetTextColor(int textId) const;
  std::wstring GetTextFont(int textId) const;

  float GetTextRotation(int textId) const;

  float GetTextSize(int textId) const;

  int GetTextEffect(int textId) const;

  int GetTextDepth(int textId) const;

  std::array<double, 2> GetTextOrigin(int textId) const;


  const std::unordered_map<int, TextToDraw>& GetCreatedTexts() const;

  int GetNumCreatedTexts() const noexcept { return texts.size(); }

private:
  TextsCollection();

private:
  uint32_t textCount;
  std::unordered_map<int, TextToDraw> texts;
};
