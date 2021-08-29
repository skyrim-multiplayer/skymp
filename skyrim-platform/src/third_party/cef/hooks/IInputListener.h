#pragma once
#include <cstdint>

class IInputListener
{
public:
  enum class MouseButton
  {
    Left = 0,
    Middle = 1,
    Right = 2
  };

  virtual void OnKeyStateChange(uint8_t scan, bool down) noexcept = 0;
  virtual void OnMouseWheel(int32_t delta) noexcept = 0;
  virtual void OnMouseMove(float deltaX, float deltaY) noexcept = 0;
  virtual void OnMouseStateChange(MouseButton mouseButton,
                                  bool down) noexcept = 0;
  virtual void OnUpdate() noexcept = 0;

  virtual ~IInputListener() = default;
};