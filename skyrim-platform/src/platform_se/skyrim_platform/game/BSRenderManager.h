#pragma once

namespace RE {
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain4;
struct ID3D11RenderTargetView1;
struct ID3D11ShaderResourceView1;

class BSRenderManager
{
public:
  static BSRenderManager* GetSingleton()
  {
    REL::Relocation<BSRenderManager**> singleton{
      Offsets::BSRenderManager::Singleton
    };
    return *singleton;
  }

  uint64_t unk00[0x48 >> 3];               // 00
  ID3D11Device* forwarder;                 // 48 - Actually CID3D11Forwarder
  ID3D11DeviceContext* context;            // 50 - ID3D11DeviceContext4
  uint64_t unk58;                          // 58
  uint64_t unk60;                          // 60
  uint64_t unk68;                          // 68
  IDXGISwapChain4* swapChain;              // 70
  uint64_t unk78;                          // 78
  uint64_t unk80;                          // 80
  ID3D11RenderTargetView1* renderView;     // 88
  ID3D11ShaderResourceView1* resourceView; // 90
  uint64_t unk2788[(0x2788 - 0x90) >> 3];
  mutable RE::BSReadWriteLock _lock; // 2790
};

class MenuScreenData
{
public:
  static MenuScreenData* GetSingleton()
  {
    REL::Relocation<MenuScreenData**> singleton{ REL::ID(403551) };
    return *singleton;
  }

  std::uint32_t unk00;    // 00
  NiPoint2 mousePos;      // 04
  float unk0C;            // 0C
  float unk10;            // 10
  float screenWidth;      // 14
  float screenHeight;     // 18
  float mouseSensitivity; // 1C
  float unk20;            // 20
  float unk24;            // 24
  float unk28;            // 28 - scaling related
  std::uint32_t unk2C;    // 2C
};
static_assert(sizeof(MenuScreenData) == 0x30);
}
