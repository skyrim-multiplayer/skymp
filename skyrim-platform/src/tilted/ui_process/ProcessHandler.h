#pragma once

#include "../core_library/Meta.hpp"
#include "OverlayRenderProcessHandler.h"

struct ProcessHandler : CEFUtils::OverlayRenderProcessHandler
{
  ProcessHandler() noexcept;
  virtual ~ProcessHandler() = default;

  TP_NOCOPYMOVE(ProcessHandler);

  void OnContextCreated(CefRefPtr<CefBrowser> browser,
                        CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefV8Context> context) override;
  void OnContextReleased(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         CefRefPtr<CefV8Context> context) override;
};
