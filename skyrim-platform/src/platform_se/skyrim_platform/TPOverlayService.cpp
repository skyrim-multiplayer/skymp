#include "TPOverlayService.h"

#include <ui/MyChromiumApp.h>

#include <D3D11Hook.hpp>

#include <ui/DX11RenderHandler.h>
#include <ui/MyRenderHandler.h>
#include <ui/ProcessMessageListener.h>
#include <ui/TextToDraw.h>

#include "TPRenderSystemD3D11.h"

#include <functional>

using CEFUtils::DX11RenderHandler;
using CEFUtils::MyRenderHandler;

struct D3D11RenderProvider final
  : MyChromiumApp::RenderProvider
  , DX11RenderHandler::Renderer
{
  explicit D3D11RenderProvider(RenderSystemD3D11* apRenderSystem)
    : m_pRenderSystem(apRenderSystem)
  {
  }

  MyRenderHandler* Create() override { return new DX11RenderHandler(this); }

  [[nodiscard]] HWND GetWindow() override
  {
    return m_pRenderSystem->GetWindow();
  }

  [[nodiscard]] IDXGISwapChain* GetSwapChain() const noexcept override
  {
    return m_pRenderSystem->GetSwapChain();
  }

private:
  RenderSystemD3D11* m_pRenderSystem;
};

OverlayService::OverlayService(
  std::shared_ptr<ProcessMessageListener> onProcessMessage_,
  std::function<std::vector<TextToDraw>()>& ObtainTextsToDraw_)
  : onProcessMessage(onProcessMessage_)
  , ObtainTextsToDraw(ObtainTextsToDraw_)
{
}

OverlayService::~OverlayService() noexcept
{
}

void OverlayService::Create(RenderSystemD3D11* apRenderSystem)
{
  auto renderProvider = std::make_unique<D3D11RenderProvider>(apRenderSystem);
  overlay = new MyChromiumApp(std::move(renderProvider), onProcessMessage);
  overlay->Initialize();
  overlay->GetClient()->Create();
}

void OverlayService::Render() const
{
  overlay->GetClient()->Render(ObtainTextsToDraw);
}

void OverlayService::Reset() const
{
  overlay->GetClient()->Reset();
}
