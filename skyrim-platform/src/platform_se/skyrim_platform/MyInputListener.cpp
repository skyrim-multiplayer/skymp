#include "MyInputListener.h"
#include "hooks/DInputHook.hpp"
#include "ui/MyChromiumApp.h"
#include <skse64/GameMenus.h>

MyInputListener::MyInputListener()
  : cursorX(*reinterpret_cast<float*>(REL::Module::BaseAddr() + 0x2F6C104))
  , cursorY(*reinterpret_cast<float*>(REL::Module::BaseAddr() + 0x2F6C108))
{
  vkCodeDownDur.fill(0);
}

void MyInputListener::Init(std::shared_ptr<OverlayService> service_,
                           std::shared_ptr<IInputConverter> conv_)
{
  service = service_;
  conv = conv_;
}

void MyInputListener::OnKeyStateChange(uint8_t code, bool down) noexcept
{
  if (!IsBrowserFocused()) {
    return;
  }

  // Switch layout if need
  bool switchLayoutDown = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
                           (GetAsyncKeyState(VK_MENU) & 0x8000)) ||
    (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
      (GetAsyncKeyState(VK_CONTROL) & 0x8000);
  if (switchLayoutDownWas != switchLayoutDown) {
    switchLayoutDownWas = switchLayoutDown;
    if (switchLayoutDown) {
      conv->SwitchLayout();
    }
  }

  // Fill vkCodeDownDur
  int virtualKeyCode = VscToVk(code);
  if (virtualKeyCode >= 0 && virtualKeyCode < vkCodeDownDur.size()) {
    vkCodeDownDur[virtualKeyCode] = down ? clock() : 0;
  }

  if (auto app = service->GetMyChromiumApp()) {
    InjectKey(code, down);

    if (down) {
      InjectChar(code);
    }
  }
}

void MyInputListener::OnMouseWheel(int32_t delta) noexcept
{
  if (!IsBrowserFocused()) {
    return;
  }

  if (auto app = service->GetMyChromiumApp()) {
    app->InjectMouseWheel(cursorX, cursorY, delta,
                          GetCefModifiersForVirtualKey(0));
  }
}

void MyInputListener::OnMouseMove(float deltaX, float deltaY) noexcept
{
  auto mm = MenuManager::GetSingleton();
  if (!mm) {
    return;
  }

  static const auto fs = new BSFixedString("Cursor Menu");
  if (!mm->IsMenuOpen(fs)) {
    return;
  }

  if (auto app = service->GetMyChromiumApp()) {
    app->InjectMouseMove(cursorX, cursorY, GetCefModifiersForVirtualKey(0),
                         IsBrowserFocused());
  }
}

void MyInputListener::OnMouseStateChange(MouseButton mouseButton,
                                         bool down) noexcept
{
  if (!IsBrowserFocused()) {
    return;
  }

  if (auto app = service->GetMyChromiumApp()) {
    cef_mouse_button_type_t btn;
    switch (mouseButton) {
      case MouseButton::Left:
        btn = cef_mouse_button_type_t::MBT_LEFT;
        break;
      case MouseButton::Middle:
        btn = cef_mouse_button_type_t::MBT_MIDDLE;
        break;
      case MouseButton::Right:
        btn = cef_mouse_button_type_t::MBT_RIGHT;
        break;
    }
    app->InjectMouseButton(cursorX, cursorY, btn, !down,
                           GetCefModifiersForVirtualKey(0));
  }
}

void MyInputListener::OnUpdate() noexcept
{
  auto mm = MenuManager::GetSingleton();
  if (!mm)
    return;
  static const auto fs = new BSFixedString("Cursor Menu");
  if (!mm->IsMenuOpen(fs)) {
    if (auto app = service->GetMyChromiumApp()) {
      app->InjectMouseMove(-1.f, -1.f, GetCefModifiersForVirtualKey(0), false);
    }
  }
  if (auto app = service->GetMyChromiumApp())
    app->RunTasks();

  // Repeat the character until the key isn't released
  for (int i = 0; i < 256; ++i) {
    const auto pressMoment = this->vkCodeDownDur[i];
    if (pressMoment && clock() - pressMoment > CLOCKS_PER_SEC / 2) {
      if (i == VK_BACK || i == VK_RIGHT || i == VK_LEFT) {
        InjectKey(MapVirtualKeyA(i, MAPVK_VK_TO_VSC), true);
        InjectKey(MapVirtualKeyA(i, MAPVK_VK_TO_VSC), false);
      } else {
        InjectChar(MapVirtualKeyA(i, MAPVK_VK_TO_VSC));
      }
    }
  }
}

bool MyInputListener::IsBrowserFocused()
{
  return CEFUtils::DInputHook::ChromeFocus();
}

void MyInputListener::InjectChar(uint8_t code)
{
  if (auto app = service->GetMyChromiumApp()) {
    int virtualKeyCode = VscToVk(code);
    int scan = code;
    auto capitalizeLetters = GetCefModifiersForVirtualKey(virtualKeyCode) &
      (EVENTFLAG_SHIFT_DOWN | EVENTFLAG_CAPS_LOCK_ON);
    auto ch = conv->VkCodeToChar(virtualKeyCode, capitalizeLetters);
    if (ch)
      app->InjectKey(cef_key_event_type_t::KEYEVENT_CHAR,
                     GetCefModifiersForVirtualKey(virtualKeyCode), ch, scan);
  }
}

void MyInputListener::InjectKey(uint8_t code, bool down)
{
  if (auto app = service->GetMyChromiumApp()) {
    int virtualKeyCode = VscToVk(code);
    int scan = code;
    app->InjectKey(down ? cef_key_event_type_t::KEYEVENT_KEYDOWN
                        : cef_key_event_type_t::KEYEVENT_KEYUP,
                   GetCefModifiersForVirtualKey(virtualKeyCode),
                   virtualKeyCode, scan);
  }
}

int MyInputListener::VscToVk(int code)
{
  int vk = MapVirtualKeyA(code, MAPVK_VSC_TO_VK);
  if (code == 203) {
    return VK_LEFT;
  }
  if (code == 205) {
    return VK_RIGHT;
  }
  return vk;
}

uint32_t MyInputListener::GetCefModifiersForVirtualKey(uint16_t aVirtualKey)
{
  uint32_t modifiers = EVENTFLAG_NONE;

  if (GetAsyncKeyState(VK_MENU) & 0x8000) {
    modifiers |= EVENTFLAG_ALT_DOWN;
  }

  if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
    modifiers |= EVENTFLAG_CONTROL_DOWN;
  }

  if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
    modifiers |= EVENTFLAG_SHIFT_DOWN;
  }

  if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
    modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
  }

  if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
    modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
  }

  if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
    modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
  }

  if (GetAsyncKeyState(VK_CAPITAL) & 1) {
    modifiers |= EVENTFLAG_CAPS_LOCK_ON;
  }

  if (GetAsyncKeyState(VK_NUMLOCK) & 1) {
    modifiers |= EVENTFLAG_NUM_LOCK_ON;
  }

  if (aVirtualKey) {
    if (aVirtualKey == VK_RCONTROL || aVirtualKey == VK_RMENU ||
        aVirtualKey == VK_RSHIFT) {
      modifiers |= EVENTFLAG_IS_RIGHT;
    } else if (aVirtualKey == VK_LCONTROL || aVirtualKey == VK_LMENU ||
               aVirtualKey == VK_LSHIFT) {
      modifiers |= EVENTFLAG_IS_LEFT;
    } else if (aVirtualKey >= VK_NUMPAD0 && aVirtualKey <= VK_DIVIDE) {
      modifiers |= EVENTFLAG_IS_KEY_PAD;
    }
  }

  return modifiers;
}