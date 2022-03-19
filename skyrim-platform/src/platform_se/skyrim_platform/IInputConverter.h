#pragma once

class IInputConverter
{
public:
  // Returns 0 on failure
  virtual wchar_t VkCodeToChar(uint8_t virtualKeyCode,
                               bool capitalLetters) noexcept = 0;
};
