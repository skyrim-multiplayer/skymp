#include "DInputHook.hpp"
#include "TextToDraw.h"
#include <DX11RenderHandler.h>
#include <DirectXColors.h>
#include <DirectXTK/DDSTextureLoader.h>
#include <DirectXTK/SimpleMath.h>
#include <DirectXTK/WICTextureLoader.h>
#include <OverlayClient.h>
#include <cmrc/cmrc.hpp>
#include <codecvt>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iterator>
#include <spdlog/spdlog.h>
#include <string>

CMRC_DECLARE(skyrim_plugin_resources);

namespace CEFUtils {
DX11RenderHandler::DX11RenderHandler(Renderer* apRenderer) noexcept
  : m_pRenderer(apRenderer)
{
  // So we need to lock this until we have the window dimension as a background
  // CEF thread will attempt to get it before we have it
  m_createLock.lock();
  isCreateLock = true;
}

DX11RenderHandler::~DX11RenderHandler() = default;

void DX11RenderHandler::Render(
  const ObtainTextsToDrawFunction& obtainTextsToDraw)
{
  // We need contexts first
  if (!m_pImmediateContext || !m_pContext) {
    Create();

    if (!m_pImmediateContext || !m_pContext)
      return;
  }

  // First of all we flush our deferred context in case we have updated the
  // texture
  {
    std::unique_lock<std::mutex> _(m_textureLock);

    Microsoft::WRL::ComPtr<ID3D11CommandList> pCommandList;
    const auto result = m_pContext->FinishCommandList(FALSE, &pCommandList);

    if (result == S_OK && pCommandList) {
      m_pImmediateContext->ExecuteCommandList(pCommandList.Get(), TRUE);
    }
  }
  GetRenderTargetSize();

  m_pSpriteBatch->Begin(DirectX::SpriteSortMode_Deferred,
                        m_pStates->NonPremultiplied());

  if (Visible()) {
    std::unique_lock<std::mutex> _(m_textureLock);

    if (m_pTextureView) {
      m_pSpriteBatch->Draw(m_pTextureView.Get(),
                           DirectX::SimpleMath::Vector2(0.f, 0.f), nullptr,
                           DirectX::Colors::White, 0.f);
    }
  }

  if (Visible()) {
    obtainTextsToDraw([&](const TextToDraw& textToDraw) {
      static_assert(
        std::is_same_v<std::decay_t<decltype(textToDraw.string.c_str()[0])>,
                       wchar_t>);

      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;

      auto& font = m_pFonts[conv.to_bytes(textToDraw.fontName)];

      if (!font)
        return;

      auto origin = DirectX::SimpleMath::Vector2(
                      font->MeasureString(textToDraw.string.c_str())) /
        2;

      DirectX::XMVECTORF32 color = { static_cast<float>(textToDraw.color[0]),
                                     static_cast<float>(textToDraw.color[1]),
                                     static_cast<float>(textToDraw.color[2]),
                                     static_cast<float>(textToDraw.color[3]) };

      font->DrawString(m_pSpriteBatch.get(), textToDraw.string.c_str(),
                       DirectX::XMFLOAT2(textToDraw.x, textToDraw.y), color,
                       textToDraw.rotation, origin, textToDraw.size,
                       textToDraw.effects, textToDraw.layerDepth);
    });
  }

  bool& focusFlag = CEFUtils::DInputHook::ChromeFocus();

  if (Visible() && focusFlag) {
    if (m_pCursorTexture && m_cursorX >= 0 && m_cursorY >= 0) {
      m_pSpriteBatch->Draw(
        m_pCursorTexture.Get(),
        DirectX::SimpleMath::Vector2(m_cursorX - 24, m_cursorY - 25), nullptr,
        DirectX::Colors::White, 0.f, DirectX::SimpleMath::Vector2(0, 0),
        m_width / 1920.f);
    }
  }

  m_pSpriteBatch->End();
}

void DX11RenderHandler::Reset()
{
  Create();
}

void DX11RenderHandler::Create()
{
  const auto hr = m_pRenderer->GetSwapChain()->GetDevice(
    IID_ID3D11Device,
    reinterpret_cast<void**>(m_pDevice.ReleaseAndGetAddressOf()));

  if (FAILED(hr))
    return;

  m_pDevice->GetImmediateContext(m_pImmediateContext.ReleaseAndGetAddressOf());

  if (!m_pImmediateContext)
    return;

  GetRenderTargetSize();

  if (FAILED(m_pDevice->CreateDeferredContext(
        0, m_pContext.ReleaseAndGetAddressOf())))
    return;

  m_pSpriteBatch =
    std::make_unique<DirectX::SpriteBatch>(m_pImmediateContext.Get());

  m_pStates = std::make_unique<DirectX::CommonStates>(m_pDevice.Get());

  if (FAILED(DirectX::CreateWICTextureFromFile(
        m_pDevice.Get(), m_pParent->GetCursorPathPNG().c_str(), nullptr,
        m_pCursorTexture.ReleaseAndGetAddressOf()))) {
    DirectX::CreateDDSTextureFromFile(
      m_pDevice.Get(), m_pParent->GetCursorPathDDS().c_str(), nullptr,
      m_pCursorTexture.ReleaseAndGetAddressOf());
  }

  cmrc::file file;
  try {
    file = cmrc::skyrim_plugin_resources::get_filesystem().open(
      "assets/cursor.png");
  } catch (std::exception& e) {
    auto dir =
      cmrc::skyrim_plugin_resources::get_filesystem().iterate_directory("");
    std::stringstream ss;
    ss << e.what() << std::endl << std::endl;
    ss << "Root directory contents is: " << std::endl;
    for (auto entry : dir)
      ss << entry.filename() << std::endl;
    throw std::runtime_error(ss.str());
  }

  DirectX::CreateWICTextureFromMemory(
    m_pDevice.Get(), reinterpret_cast<const uint8_t*>(file.begin()),
    file.size(), nullptr, m_pCursorTexture.ReleaseAndGetAddressOf());

  std::unique_lock<std::mutex> _(m_textureLock);

  if (!m_pTexture)
    CreateRenderTexture();

  for (const auto& entry :
       std::filesystem::directory_iterator("Data/Platform/Fonts/")) {
    std::filesystem::path path = entry.path();

    if (path.extension().string() != ".spritefont")
      continue;

    spdlog::info("Font has been added - " + entry.path().stem().string());

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    auto widestrFontPath =
      converter.from_bytes(static_cast<std::string>(path.string()));

    const wchar_t* fontPath = widestrFontPath.c_str();

    m_pFonts[entry.path().stem().string()] =
      std::make_unique<DirectX::SpriteFont>(m_pDevice.Get(), fontPath);
  }
}

void DX11RenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser,
                                    CefRect& rect)
{
  std::scoped_lock _(m_createLock);

  rect = CefRect(0, 0, m_width, m_height);
}

void DX11RenderHandler::OnPaint(CefRefPtr<CefBrowser> browser,
                                PaintElementType type,
                                const RectList& dirtyRects, const void* buffer,
                                int width, int height)
{
  if (type == PET_VIEW && m_width == width && m_height == height) {
    std::unique_lock<std::mutex> _(m_textureLock);

    if (!m_pTexture) {
      CreateRenderTexture();
    }

    // Under MO2 for some reason, OnPaint called before context initialization
    if (!m_pContext) {
      return;
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    const auto result = m_pContext->Map(
      m_pTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    if (SUCCEEDED(result)) {
      const auto pDest = static_cast<uint8_t*>(mappedResource.pData);
      std::memcpy(pDest, buffer, width * height * 4);
      m_pContext->Unmap(m_pTexture.Get(), 0);
    } else {
      // We got no mapping, let's drop the context and reset the texture so
      // that we attempt to create a new one during the next frame
      m_pContext.Reset();
      m_pTexture.Reset();
      m_width = m_height = 0;
    }
  }
}

void DX11RenderHandler::GetRenderTargetSize()
{
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTargetView;

  m_pImmediateContext->OMGetRenderTargets(
    1, pRenderTargetView.ReleaseAndGetAddressOf(), nullptr);
  if (pRenderTargetView) {
    Microsoft::WRL::ComPtr<ID3D11Resource> pSrcResource;
    pRenderTargetView->GetResource(pSrcResource.ReleaseAndGetAddressOf());

    if (pSrcResource) {
      Microsoft::WRL::ComPtr<ID3D11Texture2D> pSrcBuffer;
      pSrcResource.As(&pSrcBuffer);

      D3D11_TEXTURE2D_DESC desc;
      pSrcBuffer->GetDesc(&desc);

      if ((m_width != desc.Width || m_height != desc.Height) && m_pParent) {
        m_width = desc.Width;
        m_height = desc.Height;

        if (isCreateLock) {
          // We now know the size of the viewport, we can let CEF get it
          m_createLock.unlock();
          isCreateLock = false;
        }

        {
          std::unique_lock<std::mutex> _(m_textureLock);

          m_pTexture.Reset();
          m_pTextureView.Reset();
        }

        if (m_pParent->GetBrowser())
          m_pParent->GetBrowser()->GetHost()->WasResized();
      }
    }
  }
}

void DX11RenderHandler::CreateRenderTexture()
{
  D3D11_TEXTURE2D_DESC textDesc;
  textDesc.Width = m_width;
  textDesc.Height = m_height;
  textDesc.MipLevels = textDesc.ArraySize = 1;
  textDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  textDesc.SampleDesc.Count = 1;
  textDesc.SampleDesc.Quality = 0;
  textDesc.Usage = D3D11_USAGE_DYNAMIC;
  textDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  textDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  textDesc.MiscFlags = 0;

  if (FAILED(m_pDevice->CreateTexture2D(&textDesc, nullptr,
                                        m_pTexture.ReleaseAndGetAddressOf())))
    return;

  D3D11_SHADER_RESOURCE_VIEW_DESC sharedResourceViewDesc = {};
  sharedResourceViewDesc.Format = textDesc.Format;
  sharedResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  sharedResourceViewDesc.Texture2D.MipLevels = 1;

  if (FAILED(m_pDevice->CreateShaderResourceView(
        m_pTexture.Get(), &sharedResourceViewDesc,
        m_pTextureView.ReleaseAndGetAddressOf())))
    return;
}
}
