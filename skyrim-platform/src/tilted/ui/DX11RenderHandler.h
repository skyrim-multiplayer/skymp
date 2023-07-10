#pragma once

#include "MyRenderHandler.h"
#include "TextToDraw.h"
#include <DirectXTK/CommonStates.h>
#include <DirectXTK/SpriteBatch.h>
#include <DirectXTK/SpriteFont.h>
#include <Signal.hpp>
#include <functional>
#include <map>
#include <mutex>
#include <wrl.h>

struct IDXGISwapChain;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11DeviceContext;
struct ID3D11Device;

namespace CEFUtils {
struct DX11RenderHandler : MyRenderHandler
{
  static bool& Visible()
  {
    static bool g_visible = false;
    return g_visible;
  }

  struct Renderer
  {
    Renderer() = default;
    virtual ~Renderer() = default;
    [[nodiscard]] virtual IDXGISwapChain* GetSwapChain() const noexcept = 0;

    TP_NOCOPYMOVE(Renderer);
  };

  explicit DX11RenderHandler(Renderer* apRenderer) noexcept;
  virtual ~DX11RenderHandler();

  TP_NOCOPYMOVE(DX11RenderHandler);

  void Create() override;
  void Render(const ObtainTextsToDrawFunction& obtainTextsToDraw) override;
  void Reset() override;

  void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
  void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
               const RectList& dirtyRects, const void* buffer, int width,
               int height) override;

  IMPLEMENT_REFCOUNTING(DX11RenderHandler);

protected:
  void GetRenderTargetSize();
  void CreateRenderTexture();

private:
  uint32_t m_width{ 0 };
  uint32_t m_height{ 0 };

  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pTexture;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pCursorTexture;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pTextureView;
  std::mutex m_textureLock;
  std::mutex m_createLock;
  bool isCreateLock = false;
  Renderer* m_pRenderer;

  Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pContext;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pImmediateContext;

  std::unique_ptr<::DirectX::SpriteBatch> m_pSpriteBatch;

  std::unique_ptr<::DirectX::CommonStates> m_pStates;

  std::map<std::string, std::unique_ptr<::DirectX::SpriteFont>> m_pFonts;
};
}
