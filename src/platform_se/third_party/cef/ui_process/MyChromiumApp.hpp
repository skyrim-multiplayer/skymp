#pragma once

#include "OverlayRenderProcessHandler.hpp"
#include <functional>
#include <include/cef_app.h>

namespace CEFUtils {
struct MyChromiumApp final : CefApp
{
  explicit MyChromiumApp(
    const std::function<OverlayRenderProcessHandler*()>& aFactory) noexcept;
  ~MyChromiumApp() = default;

  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override;

  IMPLEMENT_REFCOUNTING(MyChromiumApp);

private:
  CefRefPtr<OverlayRenderProcessHandler> m_pRenderProcess;
};

int UIMain(
  const char* acpArgs, HINSTANCE aInstance,
  const std::function<OverlayRenderProcessHandler*()>& acFactory) noexcept;
}
