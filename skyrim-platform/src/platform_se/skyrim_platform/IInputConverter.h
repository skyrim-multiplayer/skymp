#pragma once
#include <cstdint>

class IInputConverter
{
public:
  virtual ~IInputConverter() = default;

  // Returns 0 on failure
  virtual wchar_t VkCodeToChar(uint8_t virtualKeyCode,
                               bool capitalLetters) noexcept = 0;

  // May do nothing if layouts are not supported by the implementation or there
  // is only one layout in the system
  virtual void SwitchLayout() noexcept = 0;
};
