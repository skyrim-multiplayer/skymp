#include <D3D11Hook.hpp>

#include <d3d11.h>

#include <FunctionHook.hpp>
#include <mutex>

using TDXGISwapChainPresent = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);

using TD3D11CreateDeviceAndSwapChain = HRESULT(*)(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels, UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc, IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext);

TD3D11CreateDeviceAndSwapChain RealD3D11CreateDeviceAndSwapChain = nullptr;
TDXGISwapChainPresent RealDXGISwapChainPresent = nullptr;

namespace CEFUtils
{
    HRESULT __stdcall HookDXGISwapChainPresent(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
    {
        auto& d3d11 = D3D11Hook::Get();

        static std::once_flag s_initializer;
        std::call_once(s_initializer, [&d3d11, This]()
            {
                d3d11.OnCreate(This);
            });

        d3d11.OnPresent(This);

        const auto result = RealDXGISwapChainPresent(This, SyncInterval, Flags);
        if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
        {
            d3d11.OnLost(This);
        }

        return result;
    }

    HRESULT HookD3D11CreateDeviceAndSwapChain(_In_opt_ IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, _In_opt_  const D3D_FEATURE_LEVEL* pFeatureLevels,
        UINT FeatureLevels, UINT SDKVersion, _In_opt_ const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc, _Out_opt_ IDXGISwapChain** ppSwapChain, _Out_opt_ ID3D11Device** ppDevice,
        _Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel, _Out_opt_ ID3D11DeviceContext** ppImmediateContext)
    {
        const auto result = RealD3D11CreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);

        if (RealDXGISwapChainPresent == nullptr && ppSwapChain)
            RealDXGISwapChainPresent = HookVTable(*ppSwapChain, 8, &HookDXGISwapChainPresent);

        return result;
    }

    D3D11Hook::D3D11Hook() noexcept
    {
    }

    void D3D11Hook::Install() noexcept
    {
        TP_HOOK_IAT(D3D11CreateDeviceAndSwapChain, "d3d11.dll");
    }

    D3D11Hook& D3D11Hook::Get() noexcept
    {
        static D3D11Hook s_instance;
        return s_instance;
    }
}
