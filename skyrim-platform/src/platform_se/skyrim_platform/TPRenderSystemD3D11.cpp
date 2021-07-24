#include "TPRenderSystemD3D11.h"

//#include <Services/ImguiService.h>
#include "TPOverlayService.h"

#include <D3D11Hook.hpp>
#include <d3d11.h>

/*#include <imgui.h>

#include <examples/imgui_impl_dx11.h>
#include <examples/imgui_impl_win32.h>*/

RenderSystemD3D11::RenderSystemD3D11(OverlayService& aOverlay/*,
                                     ImguiService& aImguiService*/)
  : m_pSwapChain(nullptr)
  , m_overlay(aOverlay)
//, m_imguiService(aImguiService)
{
  auto& d3d11 = CEFUtils::D3D11Hook::Get();

  m_createConnection = d3d11.OnCreate.Connect(
    std::bind(&RenderSystemD3D11::HandleCreate, this, std::placeholders::_1));
  m_resetConnection = d3d11.OnLost.Connect(
    std::bind(&RenderSystemD3D11::HandleReset, this, std::placeholders::_1));
  m_renderConnection = d3d11.OnPresent.Connect(
    std::bind(&RenderSystemD3D11::HandleRender, this, std::placeholders::_1));
}

RenderSystemD3D11::~RenderSystemD3D11()
{
  auto& d3d11 = CEFUtils::D3D11Hook::Get();

  d3d11.OnCreate.Disconnect(m_createConnection);
  d3d11.OnPresent.Disconnect(m_renderConnection);
  d3d11.OnLost.Disconnect(m_resetConnection);
}

HWND RenderSystemD3D11::GetWindow() const
{
  DXGI_SWAP_CHAIN_DESC desc{};
  desc.OutputWindow = nullptr;

  if (m_pSwapChain)
    m_pSwapChain->GetDesc(&desc);

  return desc.OutputWindow;
}

IDXGISwapChain* RenderSystemD3D11::GetSwapChain() const
{
  return m_pSwapChain;
}

void RenderSystemD3D11::HandleCreate(IDXGISwapChain* apSwapChain)
{
  m_pSwapChain = apSwapChain;

  // m_imguiService.Create(this, GetWindow());
  m_overlay.Create(this);
}

void RenderSystemD3D11::HandleRender(IDXGISwapChain* apSwapChain)
{
  m_pSwapChain = apSwapChain;
  // m_imguiService.Render();
  m_overlay.Render();
}

void RenderSystemD3D11::HandleReset(IDXGISwapChain* apSwapChain)
{
  m_pSwapChain = apSwapChain;

  m_overlay.Reset();
  // m_imguiService.Reset();
}
