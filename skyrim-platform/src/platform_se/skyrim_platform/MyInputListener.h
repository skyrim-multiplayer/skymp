#pragma once
#include "IInputConverter.h"
#include "IInputListener.h"
#include "TPOverlayService.h"

class MyInputListener : public IInputListener
{
public:
  MyInputListener();

  void Init(std::shared_ptr<OverlayService> service_,
            std::shared_ptr<IInputConverter> conv_);

  void OnKeyStateChange(uint8_t code, bool down) noexcept override;
  void OnMouseWheel(int32_t delta) noexcept override;
  void OnMouseMove(float deltaX, float deltaY) noexcept override;
  void OnMouseStateChange(MouseButton mouseButton,
                          bool down) noexcept override;
  void OnUpdate() noexcept override;

private:
  bool IsBrowserFocused();
  void InjectChar(uint8_t code);
  void InjectKey(uint8_t code, bool down);
  int VscToVk(int code);
  uint32_t GetCefModifiersForVirtualKey(uint16_t aVirtualKey);

  float& cursorX;
  float& cursorY;
  std::shared_ptr<OverlayService> service;
  std::shared_ptr<IInputConverter> conv;
  std::array<clock_t, 256> vkCodeDownDur;
  bool switchLayoutDownWas = false;
};