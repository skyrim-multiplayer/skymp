#pragma once

class TextsCollection
{
public:
  ~TextsCollection();

  int CreateText(double xPos, double yPos, std::wstring string,
                 std::array<double, 4> color);

  void DestroyText(int textId);

  void SetTextPos(int textId, float xPos, float yPos);

  void SetTextString(int textId, std::wstring str);

  void SetTextColor(int textId, std::array<double, 4> color);

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

  const std::unordered_map<int, TextToDraw>& GetCreatedTexts() const;

  int GetNumCreatedTexts() const noexcept { return texts.size(); }

private:
  TextsCollection();

private:
  uint32_t textCount;
  std::unordered_map<int, TextToDraw> texts;
};
