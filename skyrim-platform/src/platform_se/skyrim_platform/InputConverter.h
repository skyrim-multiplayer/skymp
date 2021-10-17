#pragma once
#include "IInputConverter.h"
#include <vector>

class InputConverter : public IInputConverter
{
public:
  wchar_t VkCodeToChar(uint8_t virtualKeyCode,
                       bool capitalLetters) noexcept override;
  void SwitchLayout() noexcept override;

private:
  void* keyboardLayout = nullptr;
  std::vector<void*> keyboardLayouts;
  int currentLangId = 0;
};
