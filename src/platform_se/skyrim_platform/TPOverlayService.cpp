#include "TPOverlayService.h"

#include <cef/ui/MyChromiumApp.hpp>

#include <D3D11Hook.hpp>

#include <cef/ui/MyRenderHandler.hpp>
#include <cef/ui/DX11RenderHandler.hpp>

#include "TPRenderSystemD3D11.h"

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

  MyRenderHandler* Create() override
  {
    return new DX11RenderHandler(this);
  }

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

OverlayService::OverlayService()
{
}

OverlayService::~OverlayService() noexcept
{
}

void OverlayService::Create(RenderSystemD3D11* apRenderSystem)
{
  m_pOverlay =
    new MyChromiumApp(std::make_unique<D3D11RenderProvider>(apRenderSystem));
  m_pOverlay->Initialize();
  m_pOverlay->GetClient()->Create();
}

void OverlayService::Render() const
{
  m_pOverlay->GetClient()->Render();
}

void OverlayService::Reset() const
{
  m_pOverlay->GetClient()->Reset();
}