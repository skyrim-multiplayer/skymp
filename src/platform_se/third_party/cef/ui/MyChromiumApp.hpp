#pragma once

#include "MyBrowserProcessHandler.hpp"
#include "OverlayClient.hpp"
#include <Meta.hpp>
#include <functional>
#include <include/cef_app.h>
#include <mutex>
#include <vector>

namespace CEFUtils {
struct MyChromiumApp : CefApp
{
  struct RenderProvider
  {
    RenderProvider() = default;
    virtual ~RenderProvider() = default;
    virtual MyRenderHandler* Create() = 0;
    virtual HWND GetWindow() = 0;

    TP_NOCOPYMOVE(RenderProvider);
  };

  static std::string GetCurrentSpToken();

  explicit MyChromiumApp(std::unique_ptr<RenderProvider> apRenderProvider,
                         std::wstring aProcessName =
                           L"Data/Platform/Distribution/RuntimeDependencies/"
                           L"SkyrimPlatformCEF.exe") noexcept;
  virtual ~MyChromiumApp() = default;

  TP_NOCOPYMOVE(MyChromiumApp);

  void Initialize() noexcept;
  void ExecuteAsync(const std::string& acFunction,
                    const CefRefPtr<CefListValue>& apArguments = nullptr) const
    noexcept;

  [[nodiscard]] OverlayClient* GetClient() const noexcept
  {
    return m_pGameClient.get();
  };

  void InjectKey(cef_key_event_type_t aType, uint32_t aModifiers,
                 uint16_t aKey, uint16_t aScanCode) const noexcept;
  void InjectMouseButton(uint16_t aX, uint16_t aY,
                         cef_mouse_button_type_t aButton, bool aUp,
                         uint32_t aModifier) const noexcept;
  void InjectMouseMove(float aX, float aY, uint32_t aModifier,
                       bool isBrowserFocused) const noexcept;
  void InjectMouseWheel(uint16_t aX, uint16_t aY, int16_t aDelta,
                        uint32_t aModifier) const noexcept;

  bool LoadUrl(const char* url) noexcept;

  void RunTasks();

  void OnBeforeCommandLineProcessing(
    const CefString& aProcessType,
    CefRefPtr<CefCommandLine> aCommandLine) override;

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override
  {
    return m_pBrowserProcessHandler;
  }

  IMPLEMENT_REFCOUNTING(MyChromiumApp);

private:
  CefRefPtr<MyBrowserProcessHandler> m_pBrowserProcessHandler;
  CefRefPtr<OverlayClient> m_pGameClient;
  std::unique_ptr<RenderProvider> m_pRenderProvider;
  std::wstring m_processName;

  mutable struct
  {
    mutable std::recursive_mutex m;
    std::string url;
  } share;
};
}
