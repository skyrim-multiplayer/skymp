#include "InputConverter.h"

wchar_t InputConverter::VkCodeToChar(uint8_t virtualKeyCode,
                                     bool capitalLetters) noexcept
{
  // https://github.com/cefsharp/CefSharp/issues/2143
  // https://gist.github.com/jankurianski/5b56b9e36526606bcf175747c592e1c8
  // https://stackoverflow.com/questions/6929275/how-to-convert-a-virtual-key-code-to-a-character-according-to-the-current-keyboa/6949520#6949520
  // https://github.com/adobe/webkit/blob/master/Source/WebCore/platform/chromium/KeyboardCodes.h

  wchar_t buf[128] = { 0 };

  std::array<uint8_t, 256> keyboardState;
  keyboardState.fill(0x00);
  if (capitalLetters) {
    keyboardState[VK_SHIFT] = 0xff;
  }

  // https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-tounicode
  int res;
  if (this->keyboardLayout) {
    res = ToUnicodeEx(virtualKeyCode, 0, keyboardState.data(), buf,
                      std::size(buf), 0, (HKL)this->keyboardLayout);
  } else {
    res = ToUnicode(virtualKeyCode, 0, keyboardState.data(), buf,
                    std::size(buf), 0);
  }
  if (res != 1)
    return 0;
  return buf[0];
}

void InputConverter::SwitchLayout() noexcept
{
  if (this->keyboardLayouts.empty()) {
    const int n = GetKeyboardLayoutList(0, nullptr);
    if (n <= 0)
      return;
    this->keyboardLayouts.resize(n);
    const int copied =
      GetKeyboardLayoutList(n, (HKL*)this->keyboardLayouts.data());
    if (copied != n)
      return;
  } else {
    ++this->currentLangId;
  }
  if (this->currentLangId >= this->keyboardLayouts.size()) {
    this->currentLangId = 0;
  }
  this->keyboardLayout = this->keyboardLayouts[this->currentLangId];
}
