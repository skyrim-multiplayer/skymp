#pragma once

#include <cef/core_library/Meta.hpp>
#include <include/internal/cef_ptr.h>

namespace CEFUtils {
struct MyChromiumApp;
}

struct RenderSystemD3D9;
struct RenderSystemD3D11;

using CEFUtils::MyChromiumApp;

struct OverlayService
{
  OverlayService();
  ~OverlayService() noexcept;

  TP_NOCOPYMOVE(OverlayService);

  void Create(RenderSystemD3D11* apRenderSystem);

  void Render() const;
  void Reset() const;

  MyChromiumApp* GetMyChromiumApp() const noexcept { return m_pOverlay.get(); }

private:
  CefRefPtr<MyChromiumApp> m_pOverlay{ nullptr };
};