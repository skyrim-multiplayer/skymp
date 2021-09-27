#pragma once

#include <cef/core_library/Meta.hpp>
#include <cef/ui/ProcessMessageListener.hpp>
#include <include/cef_values.h>
#include <include/internal/cef_ptr.h>

namespace CEFUtils {
struct MyChromiumApp;
}

struct RenderSystemD3D9;
struct RenderSystemD3D11;

using CEFUtils::MyChromiumApp;

struct OverlayService
{
  // onProcessMessage_ should never throw. The current behavior is
  // console.error (see OverlayClient.cpp), but you better do not throw
  // anything. Handle properly in place instead.
  OverlayService(std::shared_ptr<ProcessMessageListener> onProcessMessage_);
  ~OverlayService() noexcept;

  TP_NOCOPYMOVE(OverlayService);

  void Create(RenderSystemD3D11* apRenderSystem);

  void Render() const;
  void Reset() const;

  MyChromiumApp* GetMyChromiumApp() const noexcept { return overlay.get(); }

private:
  CefRefPtr<MyChromiumApp> overlay{ nullptr };
  const std::shared_ptr<ProcessMessageListener> onProcessMessage;
};
