#pragma once
#include <include/cef_render_handler.h>

namespace CEFUtils {
struct OverlayClient;
struct MyRenderHandler : CefRenderHandler
{
  virtual void Reset() = 0;
  virtual void Render() = 0;
  virtual void Create() = 0;

  void SetVisible(const bool aVisible) noexcept { m_visible = aVisible; }

  [[nodiscard]] bool IsVisible() const noexcept { return m_visible; }

  void SetCursorLocation(const float aX, const float aY) noexcept
  {
    m_cursorX = aX;
    m_cursorY = aY;
  }

  [[nodiscard]] std::pair<float, float> GetCursorLocation() const noexcept
  {
    return std::make_pair(m_cursorX, m_cursorY);
  }

  void SetParent(OverlayClient* apParent) noexcept { m_pParent = apParent; }

public:
  bool m_visible{ false };
  float m_cursorX{ -1.f };
  float m_cursorY{ -1.f };
  OverlayClient* m_pParent{ nullptr };
};
}
